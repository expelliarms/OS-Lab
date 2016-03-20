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
#include <sys/sem.h>
#include <errno.h>

#define COUNT 5
#define FROMMANAGER 0
#define FROMCLI 1

FILE * fp,*f;
int myID;
key_t keysem,keyque1,keyque2,keyque3;
char matrix_array[11][3];
int mID1,mID2,mID3,i,j,semID,rst;
struct sembuf wait1,signal1;

typedef struct {							//struct for message
	long from;
	char message[20];
}MESSAGE;

void terminateme(int signum)
{
    printf("ctrl + C\n");
    exit(0);
}

void file_change(int val,int queue){
    // printf("file_change(producer %d)\n",val);
    struct sembuf wait1, signal1;
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
    matrix_array[myID][queue] = val + '0';
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
	myID = atoi(argv[2]);
    printf("producerID = %d\n",myID);
    int queue,num = 0;

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
    struct timespec timout;
    timout.tv_sec=3;
    timout.tv_nsec=0;
    signal(SIGINT,terminateme);
     int tout = 0;
    while(1)
    {   
        int tnum = queue;
        queue = rand()%2;
        // queue = 0;
        if(tout)
        {
            if(tnum==0)
                queue = 1;
            else
                {
                    queue = 0;
                }
                tout = 0;
        }
        file_change(1,queue);
       printf("P%d Trying to insert in queue %d \n",myID,queue+1);
       if(queue==0){
            printf("P%d: checking if queue %d is not full\n",myID,queue+1);
            wait1.sem_num=5;//q1 full
            // semop(semID,&wait1,1);
            if(semtimedop(semID,&wait1,1,&timout)<0){
                if(errno==EAGAIN){
                    tout=1;
                    printf("timeout\n");
                    file_change(0,queue);
                    continue;
                 }
            }
            
            printf("P%d: gaining q%d\n",myID,queue+1);
            wait1.sem_num = 1;//q1 producer mutex
            semop(semID,&wait1,1);
            
            file_change(2,queue);
            
            int num=(rand()%50)+1;
            msg.from = 1;
            sprintf(msg.message,"%d",num);
            if( (rst = msgsnd(mID1,&msg,sizeof(msg),0)) < 0 ){
                perror("msg snd failed");
            }
            printf("P%d: Successfully inserted %d\n",myID,num);
            kill(managerPID,SIGUSR1);


            printf("P%d: increasing consumer check empty semaphore\n",myID);
            signal1.sem_num=6;
            semop(semID,&signal1,1);
            
            printf("P%d: releasing q%d\n",myID,queue+1);
            signal1.sem_num = 1;
            semop(semID,&signal1,1);
            file_change(0,queue);
        }
        else
        {
            printf("P%d: checking q%d is not full\n",myID,queue+1);
            wait1.sem_num=7;//q2 full
            // semop(semID,&wait1,1);
            if(semtimedop(semID,&wait1,1,&timout)<0){
                if(errno==EAGAIN){
                    tout=1;
                    printf("timeout\n");
                    continue;
                    file_change(0,queue);
                }
            }
            
            printf("P%d: gaining q%d\n",myID,queue+1);
            wait1.sem_num = 2;//q2 producer mutex
            semop(semID,&wait1,1);
            
            file_change(2,queue);
            
            int num=(rand()%50)+1;
            msg.from = 1;
            sprintf(msg.message,"%d",num);
            if( (rst = msgsnd(mID2,&msg,sizeof(msg),0)) < 0 ){
                perror("msg snd failed");
            }
            printf("P%d: Successfully inserted %d\n",myID,num);
            kill(managerPID,SIGUSR1);

            printf("P%d: increasing consumers check empty semaphore\n",myID);
            signal1.sem_num=8;
            semop(semID,&signal1,1);

            printf("P%d: releasing q%d\n",myID,queue+1);
            signal1.sem_num = 2;
            semop(semID,&signal1,1);
            
            file_change(0,queue); 
        }
    }
return 0;
}
