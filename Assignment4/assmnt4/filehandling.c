FILE * fp;
        int file=atoi(argv[1]);
        if(file==1)       fp=fopen("RR_Stats","w");
        else fp=fopen("P_RR_Stats","w");
        time_t avg_w=0,avg_t=0,avg_r=0;
        //fprintf(fp,"Process\t\tWaiting Time(microsec)\t\tTurnaround Time (s)\t\tResponse Time(microsec)\n\n");
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

typedef struct {
    long from;
    long PID;
    long priority;
    char message[20];
}MESSAGE;

void notify(int signum)
{

}

void suspend(int signum)
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
    priority = atoi(argv[2]);
    iterations = atoi(argv[1]);
    sleep_prob = atof(argv[3]);
    sleeptime = atoi(argv[4]);
    key_t key = ftok(".",'V');
    int mid = msgget(key,0);

    msg.from = FROMPROCESS;
    msg.PID = main_PID;
    msg.priority = priority;
    strcpy(msg.message,"BEGIN");

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
    signal(SIGCONT,notify);
    signal(SIGUSR2,suspend);
    pause();

    printf("Starting Process\n");


    for(i=0;i<iterations;i++)
    {
        rand_num = (rand()%100)/100.0;
        printf("rand num = %f\n",rand_num);
        if(rand_num < sleep_prob)
        {
            printf("Going for I/O\n");
            int sg = kill(sched_PID,SIGUSR1);
            if(sg<0) perror("kill");

            sleep(sleeptime);
            printf("Returning from I/O\n");
            strcpy(msg.message,"IO");
            msg.from = FROMPROCESS;
            msg.PID = main_PID;
            if(msgsnd(mid,&msg, sizeof(msg.message),0)<0){
                perror("msgsnd");
                exit(-1);
            }
            pause();
        }       
    }
    printf("Process Terminated\n");
    kill(sched_PID,SIGTERM);
    return 0;
}