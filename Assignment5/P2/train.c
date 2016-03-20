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
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define SLEEP_INTERVAL 10
#define MAX_NUM_TRAINS 50
int **matrix;

int fetch_matrix_file()
{
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

void update_matrix_file(int N, int M, int I, int J, int val)
{
  matrix[I][J] = val;
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

void fetch_and_update_file(int N, int M, int I, int J, int val)
{
  int semid_file, j;
  key_t key_file;
  key_file = 12;
  struct sembuf *sops = (struct sembuf *) malloc(2*sizeof(struct sembuf));
  if((semid_file = semget(key_file, 1, IPC_CREAT | 0660))<0)//Semaphore for ensuring mutual exclusion of file access
  {
    printf("Error Creating Junction Semaphore\n");
    exit(-1);
  }
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
    update_matrix_file(N, M, I, J, val);//Update matrix file telling that I've acquired this direction semaphore
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
    printf("Invalid arguments -- Number and Direction of the train not provided\n");
    exit(-1);
  }
  int direction, train_num;
  if(strcmp("N", argv[1]) == 0)
  {
    direction = NORTH;
  }
  else if(strcmp("E", argv[1]) == 0)
  {
    direction = EAST;
  }
  else if(strcmp("S", argv[1]) == 0)
  {
    direction = SOUTH;
  }
  else if(strcmp("W", argv[1]) == 0)
  {
    direction = WEST;
  }
  train_num = atoi(argv[2]);
  printf("Train #%d: From direction - %s\n", train_num, argv[1]);
  int semid, semid_jn;
  key_t key, key_jn;
  key = 123;
  key_jn = 1234;
  //Creating Semaphores
  if((semid = semget(key, NUMSMPHRS, 0))<0)
  {
    printf("Error Accessing Track Semaphore\n");
    exit(-1);
  }
  if((semid_jn = semget(key_jn, 1, 0))<0)
  {
    printf("Error Accessing Junction Semaphore\n");
    exit(-1);
  }
  //Initializing matrix array
  int num_trains = fetch_matrix_file();
  printf("Num Trains: %d\n", num_trains);
  int nsops, i, j;
  // printf("hi\n" );
  // for(i = 0; i < num_trains; i++)
  // {
  //   for(j = 0; j < NUMSMPHRS; j++)
  //   {
  //     // printf("hi\n" );
  //     printf("%d ", matrix[i][j]);
  //   }
  //   printf("\n");
  // }
  struct sembuf *sops = (struct sembuf *) malloc(2*sizeof(struct sembuf));
  printf("Waiting for %s Track\n", argv[1]);
  fetch_and_update_file(num_trains, NUMSMPHRS, train_num, direction, 1);//Update matrix file telling that I'm waiting for direction semaphore
  nsops = 2;// number of operations to do
  //Wait for current direction semaphore
  sops[0].sem_num = direction;//Go to semaphore corresponding to the incoming train direction
  sops[0].sem_op = 0; // wait for semaphore flag to become zero
  sops[0].sem_flg = 0; // take off semaphore asynchronous
  sops[1].sem_num = direction;
  sops[1].sem_op = 1; /* increment semaphore -- take control of track */
  sops[1].sem_flg = 0;
  /* Make the semop() call and report the results. */
  if ((j = semop(semid, sops, nsops)) == -1)
  {
    perror("semop: semop failed");
  }
  else
  {
    printf("\tsemop: semop returned %d\n", j);
    printf("\n\nChild Process Taking Control of %s Track\n", argv[1]);
    fetch_and_update_file(num_trains, NUMSMPHRS, train_num, direction, 2);//Update matrix file telling that I've acquired this direction semaphore
    fetch_and_update_file(num_trains, NUMSMPHRS, train_num, (direction + 3)%NUMSMPHRS, 1);//Update matrix file telling that I'm waiting for right of the courrent direction semaphore
    printf("Waiting for right direction Track\n");
    nsops = 2;
    //Wait for train from the right of current direction semaphore
    sops[0].sem_num = (direction + 3)%NUMSMPHRS;//Go to semaphore corresponding to the right of the current direction
    sops[0].sem_op = 0; // wait for semaphore flag to become zero
    sops[0].sem_flg = 0;
    sops[1].sem_num = (direction + 3)%NUMSMPHRS;
    sops[1].sem_op = 1; /* increment semaphore -- take control of track */
    sops[1].sem_flg = 0;
    /* Make the semop() call and report the results. */
    if ((j = semop(semid, sops, nsops)) == -1)
    {
      perror("semop: semop failed");
    }
    else
    {
      fetch_and_update_file(num_trains, NUMSMPHRS, train_num, (direction + 3)%NUMSMPHRS, 2);//Update matrix file telling that I've acquired right of the courrent direction semaphore
      printf("\tsemop: semop returned %d\n", j);
      for(i = 0; i<NUMSMPHRS; i++)
      {
        int retval=semctl(semid, i, GETVAL, 0);
        printf("semaphore[%d] = %d\n", i, retval);
      }
      nsops = 2;
      printf("Waiting for Junction\n");
      //Wait for Junction semaphore
      sops[0].sem_num = 0;//There is only one sub semaphore
      sops[0].sem_op = 0; // wait for semaphore flag to become zero
      sops[0].sem_flg = 0;
      sops[1].sem_num = 0;
      sops[1].sem_op = 1; /* increment semaphore -- take control of Junction */
      sops[1].sem_flg = 0;
      /* Make the semop() call and report the results. */
      if ((j = semop(semid_jn, sops, nsops)) == -1)
      {
        perror("semop: semop failed");
      }
      else
      {
        printf("\n\nChild Process Taking Control of Junction\n");
        sleep(SLEEP_INTERVAL); /* DO Nothing for 5 seconds */
        //Give up control of Junction semaphore
        nsops = 1;
        sops[0].sem_num = 0;
        sops[0].sem_op = -1; /* Give UP COntrol of Junction */
        sops[0].sem_flg = 0;
        if ((j = semop(semid_jn, sops, nsops)) == -1)
        {
          perror("semop: semop failed");
        }
        else
        {
          printf("Child Process Giving up Control of Junction semaphore\n");
        }
      }
      //Give up control of right direction semaphore
      nsops = 1;
      sops[0].sem_num = (direction + 3)%NUMSMPHRS;
      sops[0].sem_op = -1; /* Give UP COntrol of track */
      sops[0].sem_flg = 0;
      if ((j = semop(semid, sops, nsops)) == -1)
      {
        perror("semop: semop failed");
      }
      else
      {
        fetch_and_update_file(num_trains, NUMSMPHRS, train_num, (direction + 3)%NUMSMPHRS, 0);//Update matrix file telling that I've released the control of right of the courrent direction semaphore
        printf("Child Process Giving up Control of right direction semaphore\n");
      }
      // sleep(SLEEP_INTERVAL); /* DO Nothing for 5 seconds */
      //Give up control of the original semaphore
      nsops = 1;
      sops[0].sem_num = direction;
      sops[0].sem_op = -1; /* Give UP COntrol of track */
      sops[0].sem_flg = 0;
      if ((j = semop(semid, sops, nsops)) == -1)
      {
        perror("semop: semop failed");
      }
      else
      {
        fetch_and_update_file(num_trains, NUMSMPHRS, train_num, direction, 0);//Update matrix file telling that I've released the control of the courrent direction semaphore
        printf("Child Process Giving up Control of %s Track\n", argv[1]);
      }
      // sleep(SLEEP_INTERVAL); /* halt process to allow parent to catch semaphor change first */
    }
  }
  // printf("Semaphore state while this process exits\n");
  // for(i = 0; i<NUMSMPHRS; i++)
  // {
  //   int retval=semctl(semid, i, GETVAL, 0);
  //   printf("semaphore[%d] = %d\n", i, retval);
  // }
}
