#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#define FROMPROCESS 1L
#define FROMSCHEDULER 2L

typedef struct {							//struct for message
	long from;
	long PID;
	long priority;
	char message[20];
}MESSAGE;

void notify(int signum){}					//fucntion to begin execution
	
void suspend(int signum)					//function to pause the execution
{
	printf("Suspending the process");
	pause();
}

int main(int argc,char **argv)
{
	int iterations,priority,sleeptime,i;
	float sleep_prob,rand_num;
	pid_t main_PID = getpid(),sched_PID;		
	MESSAGE msg;
	//arguments passed by the execlp call
	priority = atoi(argv[2]);				
	iterations = atoi(argv[1]);
	sleep_prob = atof(argv[3]);
	sleeptime = atoi(argv[4]);
	key_t key = ftok(".",'V');
	int mid = msgget(key,0);

	msg.from = FROMPROCESS;
	msg.PID = main_PID;
	msg.priority = priority;
	strcpy(msg.message,"BEGI");

	if(msgsnd(mid,&msg, sizeof(msg.message) , 0) < 0){
		perror("msgsnd");
		exit(-1);
	}
	if(msgrcv(mid,&msg, sizeof(msg.message), FROMSCHEDULER, 0) < 0)
	{
		perror("msgrcv");
		exit(-1);
	}
	sched_PID = msg.PID;
	signal(SIGCONT,notify);					//initiating the function notify for sigcont signal
	signal(SIGUSR2,suspend);				//intitiating the function suspend for sigusr2 signal
	pause();								//pausing the process untill it is waked by the scheduler

	printf("Starting Process\n");			


	for(i=0;i<100;i++)
	{											
		rand_num = (rand()%100)/100.0;			
		//printf("rand num = %f\n",rand_num);	
		if(rand_num < sleep_prob)
		{
			printf("Going for I/O\n");					
			int sg = kill(sched_PID,SIGUSR1);			//sending the sigusr1 signal to the scheduler for telling IO
			if(sg<0) perror("kill");					

			sleep(sleeptime);
			printf("Returning from I/O\n");				
			strcpy(msg.message,"IO");
			msg.from = FROMPROCESS;						//signal returned from IO
			msg.PID = main_PID;							//sending the PID to the scheduler
			if(msgsnd(mid,&msg, sizeof(msg.message),0)<0){
				perror("msgsnd");
				exit(-1);
			}
			pause();
		}		
	}
	printf("Process Terminated\n");					//if all the iterations is over the process has terminated
	kill(sched_PID,SIGTERM);
	return 0;
}