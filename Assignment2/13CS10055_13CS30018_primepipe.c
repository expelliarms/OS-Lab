/* Yeshwanth V   - 13CS10055 */
/* Kshitiz Kumar - 13CS30018 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define COMPOSITE 0
#define PRIME     1
#define CAPACITY 30000

/*
Function to check if the number n is prime or cmposite
Parameters: n -- input number
Return Value: 1 if the number is prime, 0 if the number is composite
*/
int checkprime(int n)
{
	int i = 0;
	int flag = 0;
	for(i = 2;i <= (int)sqrt((double)n); i++)
	{
		if(n%i == 0)
		{
			flag = 1;//Flag 1 if the number has a divisor 
			break;
		}
	}	
	if(flag) return COMPOSITE;
	else return PRIME;		
}

int main(int argc, char *argv[])
{
	pid_t PID_main = getpid();
	if(argc!=3) 
	{
		printf("Command format: ./a.out N K\n");
		exit(0);
	}	
	/* Declarations */
	int numprime = 0;
	int N = atoi(argv[1]);
	time_t t;
	int K = atoi(argv[2]);
	int primearr[CAPACITY];
	int i = 0;
	for(i = 0;i < N; i++)
	{
		primearr[i] = 0;	
	}
	srand(time(&t));			
	/* Declare pipes */
	int parent2child[K][2];			
	int child2parent[K][2];
	for(i=0;i<K;i++)
	{
		if(pipe(parent2child[i]) < 0) perror("Failed to open pipe\n");
		if(pipe(child2parent[i]) < 0) perror("Failed to open pipe\n");		
	}
	/* Create K identical child processes */
	for(i = 0;i < K; i++)
	{ 
		if(getpid() == PID_main) fork();
		else
		{ 
			setpgid(0,PID_main+i);//Set procss group id
			break;
		}
	}
	if(getpid() != PID_main)//Child process
	{
		//printf("Hi\n");
	 	int thisPipe = getpgid(0) - PID_main;//Fetch the index of the pipe used by this child
	 	close(parent2child[thisPipe][1]);//Close unnecessary ends of the pipes
	 	close(child2parent[thisPipe][0]);
	 	int ReadEnd =	parent2child[thisPipe][0];//Define readend -- Read from Parent-to-child Pipe
	 	int WriteEnd =	child2parent[thisPipe][1];//Define writeEnd -- Write to Child-to-Parent pipe
	 	/* Define Special Signals -- available and busy */
	 	int available = 30001;
	 	int busy = 30002;
	 	write(WriteEnd,&available,sizeof(available));//Write Available Signal in the write end of the child2parnt pipe and wait for numbers
	 	//printf("%d\n",thisPipe);
	 	while(1)
	 	{
	 		int tempPrimeArr[K];//Temporary array to store all the items read from the pipe
	 		int read_from_pipe;//Temporary variable to store items read from the pipe
	 		for(i = 0; i < K; i++)
	 		{
	 			read(ReadEnd,&read_from_pipe,sizeof(read_from_pipe));//Read from the pipe
	 			//printf("Child READ %d from PARENT \n",read_from_pipe);
	 			tempPrimeArr[i] = read_from_pipe;		
	 		}
	 		write(WriteEnd,&busy,sizeof(busy));//Send Busy Signal after reading numbers
	 		for(i = 0;i < K; i++)
	 		{
	 			if(checkprime(tempPrimeArr[i]))  
	 				write(WriteEnd,&(tempPrimeArr[i]),sizeof(tempPrimeArr[i]));//Write the prime numbers into the write end of the pipe
	 		}
	 		write(WriteEnd,&available,sizeof(available));//Send Available Signal as the child is now available for furthur computation		
		}	
	}	
	else//Main parent Process
	{	
		for(i = 0; i < K; i++)
		{
			close(parent2child[i][0]);//Close unnecessary ends of the pipes
	 		close(child2parent[i][1]);
		}
		while(1)
		{
			for(i = 0;i < K;i++)//Keep Checking the Status of all children
			{
				int ReadEnd = child2parent[i][0];//Readend is now from Child-to-parent Pipe
	 			int WriteEnd =	parent2child[i][1];//WriteEnd is now in Parent-to-Child pipe
	 			int read_from_pipe;//Temporary variable
	 			read(ReadEnd,&(read_from_pipe),sizeof(read_from_pipe));
				//printf("Parent READ %d from CHILDPIPE %d\n",read_from_pipe,i);
				if(read_from_pipe == 30001)//If the read end gives Available Signal
				{
					for( i = 0;i < K; i++)
					{
						int p = (rand()%30000)+1;
						write(WriteEnd,&p,sizeof(p));//Send K numbers to child process
					}
				}
				else if (read_from_pipe	==30002) continue;//If Busy signal is obtained go to next child process
				else if (read_from_pipe <30001)//we have a number instead of a special signal 
				{
					int rep_flag=0;
					for(i = 0;i < numprime; i++)
					{
						if(read_from_pipe == primearr[numprime])//Check for repetitions
						{
							rep_flag = 1;
							break;
						}	
					}
					if(!rep_flag)
					{
						primearr[numprime] = read_from_pipe;//If there is no repetition -- update the primearr with the number
						numprime++;//Increment the number of primes detected
					}	
				}
				if(numprime >= 2*K)//If we have 2K primes -- Finish Execution. Ideally it should be N but the question description says 2K. Bit of ambiguity here
				{
					printf("Generated Prime Numbers : \n");
					for(i = 0;i < numprime; i++) 
						printf(" %d ",primearr[i]);//Print all primes found 
					printf("\n");
					for(i=K-1;i>=0;i--)
					{
						kill(-(PID_main+i),SIGKILL);//Kill all child processes
					}
					return 0;
				}
			}						
		}
	}	
	return 0;
}
