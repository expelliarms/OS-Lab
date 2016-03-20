/*
Yeshwanth -- 13CS10055
Kshitiz Kumar -- 13CS30018
*/
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
#include <sys/sem.h>
#define NUMSMPHRS 4
#define MAX_NUM_TRAINS 50
#define NUM_ITERATIONS 1000

// union semun {
//   int val;
//   struct semid_ds *buf;
//   ushort *array;
// } arg;

int **matrix;
int **RAG;
int *visited;
int *parent;

void write_matrix_file(int N, int M)
{
  //Function to update matrix file
  int i,j;
  FILE *mfp;
  mfp = fopen("matrix.txt","w");
  for(i=0;i<N;i++)
  {
    for(j=0;j<M;j++)
    {
      // matrix[i][j] = 0;
      fprintf(mfp,"%d ", matrix[i][j]);
    }
    fprintf(mfp,"\n");
  }
  fclose(mfp);
  return;
}

int fetch_matrix_file()
{
  //Function to fetch contents of matrix file
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t read;

  const char *s = " ";
  char *token = NULL;

  int i = 0;
  int j;

  fp = fopen("matrix.txt", "r");
  if (fp == NULL)
  {
    printf("Error opening");
    exit(EXIT_FAILURE);
  }
  matrix = (int **)malloc(MAX_NUM_TRAINS*sizeof(int *));
  j = 0;
  while ((read=getline(&line, &len, fp)) != -1)
  {
    matrix[j] = (int *)malloc(NUMSMPHRS*sizeof(int));
     token = strtok(line, s);
     i = 0;
     while(i < NUMSMPHRS)
     {
       matrix[j][i] = atoi(token);
      //  printf("%d ", matrix[j][i]);
       token=strtok(NULL,s);
       i++;
     }
     j++;
  }
  return j;
}

int dfs(int u,int *cycle,int *last,int prev, int num_trains)
{
  //Utility function to check cycles in the graph
  int i;
  visited[u] = 1;
  for(i = 0;i < NUMSMPHRS + num_trains;i++)
  {
    if(i != u && visited[i] == 0 && RAG[u][i] == 1)
    {
      parent[i] = u;
      return dfs(i,cycle,last,u, num_trains);//recursively check for adjacent vertices
    }
    else if(i != u && visited[i] == 1 && RAG[u][i] == 1 && i != parent[u])
    {
      int q = u;
      int len = 0;
      while( q != -1)
      {
        q = parent[q];
        len++;
      }
      if(len > 2)//Eliminate 2 edge loops
      {
        *cycle = 1;
        *last = u;
        break;
      }
    }
  }
  visited[u] = 2;
  if(*cycle == 1)
    return 1;
  else
    return 0;
}

void check_for_deadlock()
{
  //Function to check if the system is in deadlock
  int semid_file, j, i;
  key_t key_file;
  key_file = 12;
  struct sembuf *sops = (struct sembuf *) malloc(2*sizeof(struct sembuf));
  if((semid_file = semget(key_file, 1, IPC_CREAT | 0660))<0)//Semaphore for ensuring mutual exclusion of file access
  {
    printf("Error Creating Junction Semaphore\n");
    exit(-1);
  }

  //Wait for acceessing matrix file -- Semaphore ensures mutual exclusion
  int nsops = 2;
  sops[0].sem_num = 0;//There is only one sub semaphore
  sops[0].sem_op = 0; // wait for semaphore flag to become zero
  sops[0].sem_flg = 0;
  sops[1].sem_num = 0;
  sops[1].sem_op = 1; /* increment semaphore -- take control of file */
  sops[1].sem_flg = 0;
  /* Make the semop() call and report the results. */
  if ((j = semop(semid_file, sops, nsops)) == -1)
  {
    perror("semop: semop failed");
  }
  else
  {
    int num_trains = fetch_matrix_file();
    for(i = 0; i < num_trains; i++)
    {
      for(j = 0; j < NUMSMPHRS; j++)
      {
        // printf("hi\n" );
        printf("%d ", matrix[i][j]);
      }
      printf("\n");
    }
    // printf("Resource allocation graph is being created\n");
    //Create resource allocation graph
    RAG = (int **)malloc((NUMSMPHRS + num_trains)*sizeof(int *));
    for(i = 0; i < NUMSMPHRS + num_trains; i++)
    {
      RAG[i] = (int *)malloc((NUMSMPHRS + num_trains)*sizeof(int));
      for(j = 0; j < NUMSMPHRS + num_trains; j++)
      {
        RAG[i][j] = 0;
        if(i > NUMSMPHRS && j < NUMSMPHRS)
        {
          if(matrix[i - NUMSMPHRS][j] == 1)
          {
            RAG[i][j] = 1;
          }
        }
        else if(i < NUMSMPHRS && j > NUMSMPHRS)
        {
          if(matrix[j - NUMSMPHRS][i] == 2)
          {
            RAG[i][j] = 1;
          }
        }
      }
    }
    // printf("Hi\n");
    int cycle,last,prev = -1;
    parent = (int *)malloc((NUMSMPHRS + num_trains)*sizeof(int));
    visited = (int *)malloc((NUMSMPHRS + num_trains)*sizeof(int));
    for(i = 0; i < NUMSMPHRS + num_trains;i++)
    {
      parent[i] = -1;
      visited[i] = 0;
    }
    int flag = 0;
    for(i = 0; i < NUMSMPHRS + num_trains; i++)
    {
      parent[i] = -1;
      if(visited[i] == 0)
      {
        flag = dfs(i,&cycle,&last,prev, num_trains);//call Utility function dfs to check cycles in the graph
        if(flag == 1)//Deadlock detected
        {
          printf("***** Deadlock detected ******\n");
          int begin = last;
          int p = last;
          while (parent[p] != -1)
          {
            p = parent[p];
          }
          //Print cycle
          printf(" Semaphore %d is needed for ", p);
          while( parent[begin] != -1)
          {
            if(begin > 4)
            {
              printf("Train #%d;\n Train #%d is Holding ",begin - 4, begin - 4);
            }
            else
            {
              printf("Semaphore %d; which is needed for ",begin);
            }
            begin = parent[begin];
          }
          printf("\n");
          break;
        }
      }
    }
    if(flag == 0)
    {
      printf("** No deadlock detected **\n");// Deadlock not detected after doing dfs on whole graph
    }
    // update_matrix_file(N, M, I, J, val);//Update matrix file telling that I've acquired this direction semaphore
    // sleep(SLEEP_INTERVAL); /* DO Nothing for 5 seconds */
    //Give up control of File semaphore
    nsops = 1;
    sops[0].sem_num = 0;
    sops[0].sem_op = -1; /* Give UP COntrol of Junction */
    sops[0].sem_flg = 0;
    if ((j = semop(semid_file, sops, nsops)) == -1)
    {
      perror("semop: semop failed");
    }
    else
    {
      // printf("Child Process Giving up Control of File semaphore\n");
    }
  }
}

int main(int argc, char **argv)
{
  if(argc < 3)
  {
    printf("Usage : Requires an arguments --Input file having sequence of trains --Probablity with which the system shoud check deadlock condition(percent)\n");
    exit(0);
  }
  int prob_check = atoi(argv[2]);
  FILE *fp;
  if((fp=fopen(argv[1], "r")) == NULL)
  {
    printf("Cannot open file.\n");
  }
  else
  {
    printf("-------START-------\n");
    int N = 0, r, i, j, numtrains_seen = 0, trains_exhausted = 0;
    int semid, semid_jn, semid_file;
    key_t key, key_jn, key_file;
    key = 123;
    key_jn = 1234;
    key_file = 12;
    //Creating Semaphore
    if((semid = semget(key, NUMSMPHRS, IPC_CREAT | 0660))<0)//4 semaphores for 4 tracks
  	{
    	printf("Error Creating Semaphore\n");
    	exit(-1);
  	}
    if((semid_jn = semget(key_jn, 1, IPC_CREAT | 0660))<0)//Semaphore to ensure mutual exclusion of the junction
  	{
    	printf("Error Creating Junction Semaphore\n");
    	exit(-1);
  	}
    if((semid_file = semget(key_file, 1, IPC_CREAT | 0660))<0)//Semaphore for ensuring mutual exclusion of file access
  	{
    	printf("Error Creating Junction Semaphore\n");
    	exit(-1);
  	}
    //Initializing all (sub)semaphores to 0
    for(i = 0; i < NUMSMPHRS; i++)
    {
      semctl(semid, i, SETVAL, 0);
      int retval = semctl(semid, i, GETVAL, 0);
      printf("(Sub)Semaphore #%d: Value = %d\n",i, retval);
    }
    semctl(semid_jn, 0, SETVAL, 0);
    int retval = semctl(semid_jn, 0, GETVAL, 0);
    printf("Junction Semaphore Value = %d\n", retval);
    semctl(semid_file, 0, SETVAL, 0);
    retval = semctl(semid_file, 0, GETVAL, 0);
    printf("File Semaphore Value = %d\n", retval);

    char c;
    char *line = (char*) malloc(MAX_NUM_TRAINS*sizeof(char));//Read from file
    ssize_t bufsize = 0;
    r = getline(&line, &bufsize, fp);
    printf("Input Sequence: '%s'\n", line);
    int num_trains = strlen(line) - 1;
    //Initializing matrix array
    matrix = (int **)malloc(num_trains*sizeof(int *));
    for(i=0;i<num_trains;i++)
    {
      matrix[i] = (int *)malloc(NUMSMPHRS*sizeof(int));
      for(j=0;j<NUMSMPHRS;j++)
      {
        matrix[i][j] = 0;
      }
    }
    write_matrix_file(num_trains, NUMSMPHRS);//Update file
    i = 0;
    srand ( time(NULL) );
    while(i < NUM_ITERATIONS)
    {
      sleep(1);
      int random_num = rand()%100;
      printf("---------Iteration #%d: Random number generated: %d---------\n", i, random_num);
      for(j = 0; j<NUMSMPHRS; j++)
      {
        int retval=semctl(semid, j, GETVAL, 0);
        printf("semaphore[%d] = %d\n", j, retval);
      }
      if(random_num < prob_check || trains_exhausted == 1)
      {
        //Code to check deadlock
        printf("\tChecking for deadlock\n");
        check_for_deadlock();
      }
      else
      {
        //Next train
        if(line[numtrains_seen] != '\n')
        {
          printf("\tCreating next train process\n");
          char parameter1[15], parameter2[15];
          sprintf(parameter1, "%c", line[numtrains_seen]);//Set parameter1 to the direction of the train
          sprintf(parameter2, "%d", numtrains_seen);//Set parameter1 to the direction of the train
          if(fork() == 0)
          {
            //Child process opens train in new xterm window
            int execret = execlp("xterm","xterm","-hold","-e","./train", parameter1,parameter2,(const char*) NULL);
            if(execret <0 ) perror("Error in exec");
              exit(0);
          }
          printf("\t\t--- %c train started\n", line[numtrains_seen]);
          numtrains_seen++;
        }
        else
        {
          printf("\tTrains Exhausted -- All subsequent iterations will only check for deadlock condition\n");
          trains_exhausted = 1;
        }
      }//end else
      i++;
    }//End while
  }//End Else
  fclose(fp);
}
