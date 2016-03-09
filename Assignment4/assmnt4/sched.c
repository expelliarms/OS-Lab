#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#define FROMPROCESS 1L
#define FROMSCHEDULER 2L
#define SIZE 500
#define TIME_QUANTA 100

int process_count=-1, ioPID=-1, count=0, term_flag =0;

typedef struct {
    long    from;
    long    PID;
    long    priority;
    char    message[20];
} MESSAGE;

typedef struct {
  int index;
  pid_t PID;
  int priority;
  int has_response;
  time_t arrival_time;
  struct timeval inRQ;
  unsigned long response_time;
  unsigned long waiting_time;
  time_t turnaround_time;
} PROC;

void error(char *ch)
{
	printf("%s\n",ch);
	exit(-1);
}

void toIO(int signo, siginfo_t *info, void *extra) 			
{
      ioPID=info->si_pid;
}
void terminate(int signum)       
{
      term_flag=1;
}
struct timeval timenow() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv;
}

int empty(PROC *p[50])
{
	if(process_count <= 0)
	{
		return 1;
	}
	return 0;
}

void push(PROC *pr,PROC *p[50])
{
	int i,j;
	if(process_count >= SIZE-1) 
		printf("Ready queue is already full");
	else
	{
		if(process_count == 0)
		{
			process_count++;
			p[process_count] = pr;
			return;
		}
		for(i=0;i<=process_count;i++){
        	if(pr->priority >= p[i]->priority)
      		{
      			for(j=process_count+1;j>i;--j)
      				{
      					p[j] = p[j-1]; 
      				}
      				p[i] = pr;
      				process_count++;
      				return ;
      		}
  		}
  		p[i] = pr;
  		process_count++;
	}
}

PROC * pop(PROC * p[50])
{
	int i=0;
	if(empty(p))
	{
		printf("Ready queqe is empty\n");
		return NULL;
	}
	PROC * pro = NULL;
	PROC * toreturn = malloc(sizeof(PROC));
	pro = p[0];
	*toreturn = *pro;
	for(i=0;i<=process_count;++i)
	{
		p[i] = p[i+1];
	}
	process_count--;
	//printf("process returned = %d\n",toreturn->index);
	return toreturn;

}

int main(char argc,char ** argv)
{
  if(argc < 2)
  {
    printf("Incorrect Arguments \n");
    printf("Correct Format - Enter 1 for Regular Round Robin Scheduling 2 for priority Round Robin Scheduling\n");
    exit(0);
  }
  PROC* readyQueue[50];							//queqe containing the list of all processess
  PROC* ProcessList[50];						//processess in the ready queue
  int terminated=0;								//number of terminated processess
  int type = atoi(argv[1]);						//type of the scheduler 
  pid_t main_PID = getpid();					//PID of the scheduler
  int mid,read,rc,num_msg,k;
  key_t key;
  MESSAGE msg;
  struct msqid_ds msglist;
  struct sigaction action;						

  action.sa_flags = SA_SIGINFO;					
  action.sa_sigaction = &toIO;					

  sigaction(SIGUSR1,&action,NULL);
  signal(SIGTERM,terminate);

  key = ftok(".",'V');
  if((mid = msgget(key, IPC_CREAT | 0660))<0)
	{
	printf("Error Creating message queue\n");
	exit(-1);
	}

	msgctl(mid, IPC_RMID, NULL);

	if((mid = msgget(key, IPC_CREAT | 0660))<0){
            printf("Error Creating Message Queue\n");
            exit(-1);
            }

  while(1)
  {
  	//printf("Entered outer while loop");
  	rc = msgctl(mid,IPC_STAT,&msglist);                      //sense the message queue
  	num_msg = msglist.msg_qnum;								//get the number of message
  	if(num_msg > 0)											
  	{
  		for(k=0;k<num_msg;++k)
  		{
  			if(msgrcv(mid,&msg, sizeof(msg.message),FROMPROCESS,0) < 0)
  			{
  				error("msgrcv");
  			}
  			if(strcmp(msg.message,"BEGI") == 0)
  			{													//if the process is coming for the first time
  				PROC * P = malloc (sizeof(PROC));				
  				P->index = count;								//store the index of the process
  				count++;											
  				P->PID = msg.PID;								//set the PID of process
  				//if its RRR then set the priority to 1 for all process
  				if(type == 1) P->priority=1;					
  				else
  					P->priority = msg.priority;
  				msg.from = FROMSCHEDULER;
  				msg.PID=main_PID;
  				strcpy(msg.message,"REC");
  				if(msgsnd(mid, &msg, sizeof(msg.message), 0)<0)
  				{	
                   error("msgsnd");
          }

                 P->arrival_time=time(NULL);
                 P->waiting_time=0;
                 P->turnaround_time=0;
                 P->response_time=0;
                 P->has_response=0;

                 printf("Process P %d , PID = %d arrived\n",P->index,P->PID);
                 P->inRQ=timenow();
                 ProcessList[P->index]=P;
                 push(P,readyQueue);              
  			}
  			else
  				if(!strcmp(msg.message,"IO"))
  				{
  					 PROC * P = malloc (sizeof(PROC));
                     int j=0;
                     for(j=0;j<count;j++)
                     {
                        if (ProcessList[j]->PID==msg.PID)
                           {
                        		P=ProcessList[j];
                     	      	break;
                            }
                      }

                        printf("%d : Process P%d , PID = %d returned from IO\n",(int)time(NULL),P->index,P->PID);
                        P->inRQ=timenow();
                        push(P,readyQueue);             

  				}
  		}
  	}
  	if(empty(readyQueue) == 0)     
             {
             	//printf("RQ not empty condition\n");
                       int in_io=0;
                        int q=0;
                       PROC * P = pop(readyQueue);            

                       struct timeval cur=timenow();
                       struct timeval rqt=(P->inRQ);
                       unsigned long tv= (cur.tv_sec-rqt.tv_sec) + 1000000*(cur.tv_usec-rqt.tv_usec);
                       P->waiting_time += tv;
                       if(P->has_response==0)			//setting the response flag to 1 after first IO operation
                       {							
                             P->response_time=P->waiting_time;
                             P->has_response=1;
                       }
                       kill(P->PID,SIGCONT);

                	   //printf("Process P %d is running\n",P->index);

                    for(q=0;q<TIME_QUANTA;q++)
                    {
                          if(ioPID==P->PID)
                          {
                                 ProcessList[P->index]=P;

                                 printf("%d  : Process P%d , PID = %d requesting I/O\n",(int)time(NULL),P->index,ioPID);
                                 ioPID=-1;
                                 in_io=1;
                                 break;
                          }

                          if(term_flag==1)
                          {
                                ProcessList[P->index]=P;
                                P->turnaround_time=time(NULL)-(P->arrival_time);
                                printf("%d  : Process P%d , PID = %d Terminated\n",(int)time(NULL),P->index,P->PID);
                                term_flag=0;
                                terminated++;
                                break;
                          }

                    }

                    if(terminated==count) break;     

                    

             }
  }
   FILE * fp;
        int file=atoi(argv[1]);
        if(file==1)       fp=fopen("RR_Stats","w");
        else fp=fopen("P_RR_Stats","w");
        time_t avg_w=0,avg_t=0,avg_r=0;
        int l=0;
        for(l=0;l<count;l++)
        {
             fprintf(fp,"Process P%d\n Waiting Time(microsec): %lu\nTurnaround Time (s): %d Response Time(microsec): %d\n\n",l,ProcessList[l]->waiting_time,(int)ProcessList[l]->turnaround_time,(int)ProcessList[l]->response_time);
             avg_w += ProcessList[l]->waiting_time;
             avg_t += ProcessList[l]->turnaround_time;
             avg_r += ProcessList[l]->response_time;
        }
        avg_w/=count;
        avg_t/=count;
        avg_r/=count;

        fprintf(fp,"Average\n Avg Waiting Time(microsec): %lu\nTurnaround Time (s): %d\nResponse Time(microsec): %d\n",avg_w,(int)avg_t,(int)avg_r);
        printf("\nFinished\n");
      fclose(fp);
       return 0;
}
