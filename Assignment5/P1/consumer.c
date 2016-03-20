#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <mqueue.h>

#define COUNT 5
#define FROMMANAGER 0
#define FROMCLI 1


key_t keysem,keyque1,keyque2,keyque3;
int mID1,mID2,mID3,rst,type,semID,i,j;
struct sembuf wait1,signal1;
int myID;
FILE * fp,*f;
char matrix_array[11][3];

typedef struct {							//struct for message
	long from;
	char message[20];
}MESSAGE;

void file_change(int val,int queue){
    struct sembuf wait1, signal1;
     // printf("file_change(consumer %d)\n",val);
    //matrix.txt
    wait1.sem_num=0;
    wait1.sem_op=-1;
    wait1.sem_flg=0;

    signal1.sem_num=0;
    signal1.sem_op=1;
    signal1.sem_flg=0;

    semop(semID,&wait1,1);

    fp = fopen("matrix.txt", "r");
        if(fp == NULL){
            perror("fopen");
        }
        ssize_t read;
        char *line = NULL;
        size_t len = 0; 
        i=0,j=0;
        while ((read=getline(&line, &len, fp)) != -1)
        {
            for(i=0;i<2;++i)
            {
                matrix_array[j][i] = line[i];
            }
            j++;
        }
    fclose(fp);
    matrix_array[myID + COUNT][queue] = val + '0';
    f = fopen("matrix.txt","w");
    for(i=0;i<2*COUNT;++i)
    {
        fprintf(f, "%s\n", matrix_array[i]);
    }
    fclose(f);
    semop(semID,&signal1,1);
}

int main(int argc,char **argv)
{
    srand(time(NULL));
	MESSAGE msg;
	//arguments passed by the execlp call
    pid_t myPID = getpid(),managerPID;
    managerPID = atoi(argv[1]);
    float prob = ((rand()%7)+2)/10;
	myID = atoi(argv[2]);
	printf("ConsumerID = %d\n",myID);
    int queue,num = 0;
    type = atoi(argv[3]);
    msg.from = 1;
    strcpy(msg.message,"BEGI");

    keysem = ftok(".",'M');
    int  nsem = 9;
    semID = semget(keysem,nsem,0);
    if(semID < 0)
    {
        printf("Error in producer semaphore\n");
        exit(-1);
    }
    keyque1 = ftok(".", '1');
    if((mID1 = msgget(keyque1, 0))<0)
    {
    printf("Error getting message queue1\n");
    exit(-1);
    }
    keyque2 = ftok(".",'2');
    if((mID2 = msgget(keyque2, 0))<0)
    {
        printf("Error getting message queue2\n");
        exit(-1);
    }
    wait1.sem_op = -1;
    wait1.sem_flg = 0;

    signal1.sem_op = 1;
    signal1.sem_flg = 0;
    while(1)
    {
		float f1 = (rand()%101)/100.0;
		int queue=rand()%2;
		file_change(1,queue);
		
		if(f1 > prob){
			//single queue
			printf("C%d: Trying to consume from q%d\n",myID,queue+1);
			if(queue)
			{
				wait1.sem_num=8;//q2 empty
				semop(semID,&wait1,1);
				
				wait1.sem_num = 4;//q2 consumer mutex
				semop(semID,&wait1,1);
				
				file_change(2,queue);
				if((rst = msgrcv(mID2,&msg,sizeof(msg),1,IPC_NOWAIT)) < 0 ){
			        perror("failed");
			    }
				printf("C%d: Successfully consumed\n",myID);
			    printf("C%d: Received message from q2 %s\n",myID,msg.message);
				kill(managerPID,SIGUSR2);

				signal1.sem_num=7;
				semop(semID,&signal1,1);
				
				printf("C%d: releasing q%d\n",myID,queue+1);
				signal1.sem_num = 4;
				semop(semID,&signal1,1);
				
				file_change(0,queue);
			}
			else
			{
				wait1.sem_num=6;//q1 empty
				semop(semID,&wait1,1);
				
				wait1.sem_num = 3;//q1 consumer mutex
				semop(semID,&wait1,1);
				
				file_change(2,queue);
				if( (rst = msgrcv(mID1,&msg,sizeof(msg),1, IPC_NOWAIT)) < 0 ){
			        perror("msg failed");
			    }
				printf("C%d: Successfully consumed\n",myID);
				printf("C%d: Received message from q1-> %s\n",myID,msg.message);
				kill(managerPID,SIGUSR2);

				signal1.sem_num=5;
				semop(semID,&signal1,1);	
				
				printf("C%d: releasing q%d\n",myID,queue+1);
				signal1.sem_num = 3;
				semop(semID,&signal1,1);
				
				file_change(0,queue);
				
							
			}
		}
		else
		{
			if(!type)queue=0;
			printf("c%d: Trying to consume from q%d\n",myID,queue+1);
			if(queue == 0)
			{
				wait1.sem_num=6;//q1 empty
				semop(semID,&wait1,1);
				
				wait1.sem_num = 3;//q1 consumer mutex
				semop(semID,&wait1,1);
				
				file_change(2,queue);
				if( (rst = msgrcv(mID1,&msg,sizeof(msg),1, IPC_NOWAIT)) < 0 ){
			        perror("msg failed");
			    }
				printf("c%d: Successfully consumed\n",myID);
				printf("c%d: Received message from q1-> %s\n",myID,msg.message);
				kill(managerPID,SIGUSR2);

				signal1.sem_num=5;
				semop(semID,&signal1,1);

				//sleep(2);
				
				/*second queue*/
				queue=1;
				printf("c%d: Trying to consume from next queue\n",myID);
				file_change(1,queue);
				printf("c%d: checking q%d is not empty\n",myID,queue+1);
				wait1.sem_num=8;//q2 empty
				semop(semID,&wait1,1);
				
				wait1.sem_num = 4;//q2 consumer mutex
				semop(semID,&wait1,1);
				
				file_change(2,queue);
				printf("c%d: Successfully consumed\n",myID);
				if( (rst = msgrcv(mID2,&msg,sizeof(msg),1, IPC_NOWAIT)) < 0 ){
			        perror("failed");
			    }
			    printf("c%d: Received message from q2-> %s\n",myID,msg.message);
				kill(managerPID,SIGUSR2);

				signal1.sem_num=7;
				semop(semID,&signal1,1);
				
				printf("c%d: releasing q%d\n",myID,queue+1);
				signal1.sem_num = 4;
				semop(semID,&signal1,1);
				
				file_change(0,queue);
				

				/*second queue consumption ends*/

				queue=0;

				printf("c%d: releasing q%d\n",myID,queue+1);
				signal1.sem_num = 3;
				semop(semID,&signal1,1);
				
				file_change(0,queue);
				
			}

			else
			{
				wait1.sem_num=8;//q2 empty
				semop(semID,&wait1,1);
				
				wait1.sem_num = 4;//q2 consumer mutex
				semop(semID,&wait1,1);
				
				file_change(2,queue);
				printf("c%d: Successfully consumed\n",myID);
				if( (rst = msgrcv(mID2,&msg,sizeof(msg),1, IPC_NOWAIT)) < 0 ){
			        perror("failed");
			    }
			    printf("c%d: Received message from q2-> %s\n",myID,msg.message);
				kill(managerPID,SIGUSR2);

				signal1.sem_num=7;
				semop(semID,&signal1,1);
				

				//sleep(2);

				queue=0;
			    /*q1 starts*/
			    printf("C%d: Trying to consume from next queue\n",myID);
				file_change(1,queue);
				printf("C%d: checking q%d is not empty\n",myID,queue+1);
				wait1.sem_num=6;//q1 empty
				semop(semID,&wait1,1);
				
				wait1.sem_num = 3;//q1 consumer mutex
				semop(semID,&wait1,1);
				
				file_change(2,queue);
				printf("C%d: Successfully consumed\n",myID);
				if( (rst = msgrcv(mID1,&msg,sizeof(msg),1, IPC_NOWAIT)) < 0 ){
			        perror("msg failed");
			    }
				printf("C%d: Received message from q1-> %s\n",myID,msg.message);
				kill(managerPID,SIGUSR2);

				signal1.sem_num=5;
				semop(semID,&signal1,1);

				printf("C%d: releasing q%d\n",myID,queue+1);
				signal1.sem_num = 3;
				semop(semID,&signal1,1);
				
				file_change(0,queue);
				
				
				/*q1 consumption ends*/

				queue=1;
				printf("C%d: releasing q%d\n",myID,queue+1);
				signal1.sem_num = 4;
				semop(semID,&signal1,1);
				
				file_change(0,queue);		
			}
		}
	}
return 0;
}
