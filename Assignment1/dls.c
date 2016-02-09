/* Yeshwanth V   - 13CS10055 */
/* Kshitiz Kumar - 13CS30018 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

static pid_t PID_main=0;
FILE *fp;
#define CAPACITY 1000
int glblArray[CAPACITY];//Array is declared globally

void dls(int start,int end,int num)//Distribted linear search function -- takes the parameters start and end indices and the number to be searched and searches in the global array glblArray[]
{
	printf("Function call: dls(%d,%d)\n",start,end);
	int i = 0;
	int length = end-start;
	if(length <= 5)//Base condition -- when the array size is less than or equal to 5
	{
		for(i = start; i < end; i++)
		{	
			printf("Comparison with (%d)%d\n",i,glblArray[i]);
			if(glblArray[i] == num) 
			{	
				union sigval value;
				value.sival_int = i;	
				sigqueue(PID_main,SIGUSR1,value);	//Send a user defined signal to main when the element is found
			}
		}
		if(getpid() == PID_main) 
			sleep(1);
		else 
			exit(0);
	}
	else
	{
		pid_t child = fork();		//create a child process and recursively call dls function
		if(child != 0) 
		{
			dls(start,(start+end)/2,num);//dls function to search in the left half of the aray
		}
		else
		{ 
			dls((start+end)/2,end,num);//dls function to search in the right half of the aray
			setpgid(0, PID_main);		//Add child process pid to process group
		}
	}
}

void printans(int signo, siginfo_t *info, void *extra)//When the number is found this function is called by an user defined signal -- prints the answer and kills all the child processes
{
	printf("Function call: printans()\n");
	int ans;
	ans = info->si_value.sival_int;//
	printf("\nAnswer: Index = %d \n",ans);
	fclose(fp);
	kill(0, SIGTERM);
  sleep(1);
  kill(0, SIGKILL);
	return;
}

int main(int argc, char *argv[])
{
	const char *filename;
	filename = (char *)malloc(20*sizeof(char));
	filename = argv[1];
	int num = atoi(argv[2]);
	if(argc!=3) 
	{
		printf("Incorrect Arguments");
		exit(0);
	}
	printf("filename:%s\nNumber to be searched:%d\n", filename,num);		
	PID_main = getpid();//Store the pid of main process in global variable
	setpgid(0, 0);
	fp = fopen(filename,"r");
	int i = 0;
	int buffer;
	printf("------------Printing Input Array------------\n");
	while(fscanf(fp,"%d",&buffer) != EOF )//Read the array fron the file and store it in a global array
	{
		glblArray[i] = buffer;
		printf("(%d)%d ", i,glblArray[i]);
		i++;
	}
	printf("\n");
	int N = i;
	struct sigaction action;
  action.sa_flags = SA_SIGINFO;
  action.sa_sigaction = &printans;//assign function to be called when the element is found
  sigaction(SIGUSR1, &action, NULL);//Handles the signal sent when the element is found
	dls(0,N,num);
	printf("Queried number Not found\n");//If the number is found the funcion printans will kill all the process and gives the index of the number else the program control comes here
	fclose(fp);
	return 0;
}