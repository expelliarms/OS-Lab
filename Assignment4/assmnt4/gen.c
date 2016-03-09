//13CS30018 Kshitiz Kumar
//13CS10055 Yeshwanth V

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


int main()
{
    FILE * fp = fopen("All_process","r");                   //open the process description FILE
    int priority,num_of_iteration,sleep_time,r,num,interval;    
    float sleep_prob;   
    char *line = (char*) malloc(20*sizeof(char));
    ssize_t bufsize = 0;
    r = getline(&line, &bufsize, fp);                       
    sscanf(line,"%d %d",&num,&interval);
    printf("number of process  = %d time interval between processes = %d\num",num,interval);
    while((r = getline(&line,&bufsize,fp)) != -1)
    {
      sscanf(line,"%d %d %f %d",&num_of_iteration,&priority,&sleep_prob,&sleep_time);
        
        printf("Process with Number of Iterations = %d, priority = %d, sleep Probablity = %f, sleep Time  =%d\num",num_of_iteration,priority,sleep_prob,sleep_time);     
        char parameter1[15],parameter2[15],parameter3[15],parameter4[15];
        sprintf(parameter1,"%d",num_of_iteration);
        sprintf(parameter2,"%d",priority);
        sprintf(parameter3,"%f",sleep_prob);
        sprintf(parameter4,"%d",sleep_time);
        if(fork() == 0)
        {
            int execret = execlp("xterm","xterm","-hold","-e","./process",parameter1,parameter2,parameter3,parameter4,(const char*) NULL);
            if(execret <0 ) perror("Error in exec");
                exit(0);
        }
        sleep(interval);
    }
fclose(fp);
return 0;
}
