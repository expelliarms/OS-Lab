#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

int num[30005],k,available[30000],cpida[30000],numprime,pfds1[30000][2],pfds2[3000][2],count;


bool isprime(int n)
{
	int i;
	if(n == 2)
		return true;
	float l = sqrt((float)n);
	for(i=2;i<=l;++i)
	{
		if(n%i == 0)
			return false;
	}
	return true;
}
void sig_usr1(int sig, siginfo_t *s, void *no_use)
{
	int i;
	pid_t sender_pid = s->si_pid;
	for(i=0;i<k;++i)
	{
		if(cpida[i] == sender_pid)
		{
			available[i] = 1;
		}
	}

}
void sig_usr2(int sig, siginfo_t *s, void *no_use)
{
	int i;
	pid_t sender_pid = s->si_pid;
	for(i=0;i<k;++i)
	{
		if(cpida[i] == sender_pid)
		{
			available[i] = 0;
		}
	}

}
int main(int argc, char const *argv[])
{
	memset(available,0,sizeof(available));
	printf("%d",available[2]);
	int mainPID = getpid(); 
	count = 0;
	char buffer[6];
	struct sigaction action1,action2;
    action1.sa_flags = SA_SIGINFO;
    action1.sa_sigaction = &sig_usr1;
    sigaction(SIGUSR1, &action1, NULL);

    action2.sa_flags = SA_SIGINFO;
    action2.sa_sigaction = &sig_usr2;
    sigaction(SIGUSR2, &action2, NULL);
	
	int cpid;
	printf("Enter the number of primes\n");
	scanf("%d",&numprime);
	printf("Enter the number of child process\n");
	scanf("%d",&k);
	int i=0,j=0;
	for(i=0;i<k;++i)
	{
		pipe(pfds1[i]);
		pipe(pfds2[i]);
		if(getpid() == mainPID) fork();
		else
		{
			cpid = getpid();
			kill(mainPID,SIGUSR1);
			break;
		}

	}
	if(getpid() != mainPID)
	{
		int readarray[k+1],read_num;
		for(i=0;i<k;++i)
		{
			read(pfds1[i][0],&read_num,sizeof(read_num));
			readarray[i] = read_num;
		}
		kill(mainPID,SIGUSR2);
		for(i=0;i<k;++i) if(isprime(readarray[i])) write(pfds2[i][1],&(readarray[i]),sizeof(readarray[i]));
		kill(mainPID,SIGUSR1);
	}
	else
	 if(getpid() == mainPID)
		{
			for(i=0;i<k;++i)
				printf("pid = %d\n",cpida[i]);
			while(1)
			{
				for(i=0;i<k;++i)
				{
					if(available[i] == 1)
					{
						for(j=0;j<k;++j)
						{
							int random = (rand())%30001;
							write(pfds1[i][1],&random,sizeof(random));
						}
					}
				}
				for(i=0;i<k;++i)
				{
					for(j=0;j<k;++j)
					{
						int read_num;
						read(pfds2[i][0],&read_num,sizeof(read_num));
						if(num[read_num] != 1)
						{
							count++;
							num[read_num] = 1;
							if(count >= 2*k)
							{
								int l;
								for(l=0;l<k;++l)
								{
									kill(cpida[i],SIGKILL);
								}
								break; 	
							}
						}
					}
				}
			}
		}
		for(i=0;i<30000;++i)
		{
			if(num[i] == 1)
			{
				printf("%d ",num[i]);
			}
		}
		printf("\n");
	return 0;
}