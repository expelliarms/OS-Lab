/*
Yeshwanth -- 13CS10055
Kshitiz Kumar -- 13CS30018
*/

//To be compiled with -pthread
//Command: gcc -pthread 13CS10055_13CS30018_fw.c -o whatever

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#define INF 10000

int **Graph;//Adjacency matrix of the graph
int **Weights;//Edge weights
int **dist;//All pairs shortes distance matrix
int N, M;
pthread_mutex_t mutexdist_wr;//mutex variable for write operation
pthread_mutex_t mutexdist_rd;//mutex variable for write operation
int count_rdrs = 0;//Keep count of number of readers

/* --- Structure to send arguments to threads --- */
struct thread_data{
  int  k;
  int  i;
};

void *jloop(void *data)
{
  struct thread_data *args;
  args = (struct thread_data *)data;//typecast the argument into struct thread_data type
  int k, i, j;
  k = args->k;//get k from args
  i = args->i;//get i from args
  // printf("Hello World! It's me, thread k = %d, i = %d!\n", k, i);
  for(j = 1; j <= N; j++)
  {
    pthread_mutex_lock (&mutexdist_rd);//lock the read mutex
    count_rdrs++;//Increment readers count once someone enters here
    printf("Thread with k = %d, i = %d, j = %d is reading... \n", k, i, j);
    if(count_rdrs == 1)
    {
      pthread_mutex_lock (&mutexdist_wr);//The first reader locks the write mutex to make sure no one is writing while a read operation is taking place
    }
    pthread_mutex_unlock (&mutexdist_rd);//Unlock read mutex so that multiple readers can read
    sleep(2);
    if(dist[i][k] + dist[k][j] < dist[i][j])
    {
      pthread_mutex_lock (&mutexdist_rd);//lock the read mutex so that no one else is reading while I perform a write operation
      count_rdrs--;
      if(count_rdrs == 0)
      {
        pthread_mutex_unlock (&mutexdist_wr);//The last reader should unlock write mutex so that others can write
      }
      pthread_mutex_unlock (&mutexdist_rd);//Unlock read mutex as the read operation is over
      /* ---- Write to dist array done in mutually  exclsive manner ---- */
      pthread_mutex_lock (&mutexdist_wr);
      printf("Thread with k = %d, i = %d, j = %d is writing...\n", k, i, j);
      sleep(3);
      dist[i][j] = dist[i][k] + dist[k][j];
      pthread_mutex_unlock (&mutexdist_wr);
      /* --------------------------------------------------------------- */
    }
    else
    {
      pthread_mutex_lock (&mutexdist_rd);//lock the read mutex so that no one else is reading while I perform a write operation
      count_rdrs--;
      if(count_rdrs == 0)
      {
        pthread_mutex_unlock (&mutexdist_wr);//The last reader should unlock write mutex so that others can write
      }
      pthread_mutex_unlock (&mutexdist_rd);//Unlock read mutex as the read operation is over
    }
  }
  pthread_exit(NULL);
}

int main()
{
  printf("-----------START-----------\n" );
  /*--------------------------- Initialization ---------------------------*/
  int i, j, k;
  scanf("%d %d", &N, &M);
  Graph = (int **)malloc((N+1) * sizeof(int *));
  Weights = (int **)malloc((N+1) * sizeof(int *));
  dist = (int **)malloc((N+1) * sizeof(int *));
  for(i = 1; i <= N; i++)
  {
    Graph[i] = (int *)malloc((N+1) * sizeof(int));
    dist[i] = (int *)malloc((N+1) * sizeof(int));
    Weights[i] = (int *)malloc((N+1) * sizeof(int));
    for(j = 1; j <= N; j++)
    {
      Graph[i][j] = 0;
      if(i != j)
        Weights[i][j] = INF;
      else
        Weights[i][j] = 0;
      if(i != j)
        dist[i][j] = INF;
      else
        dist[i][j] = 0;
    }
  }
  /*----------------------------------------------------------------------*/
  /*------------------------ Take Input Graph ----------------------------*/
  int u, v, w;
  for(i = 0; i < M; i++)
  {
    scanf("%d %d %d", &u, &v, &w);
    Graph[u][v] = 1;
    Graph[v][u] = 1;
    dist[u][v] = w;
    dist[v][u] = w;
    Weights[u][v] = w;
    Weights[v][u] = w;
  }
  /*----------------------------------------------------------------------*/
  /*---------------------- Single threaded version -----------------------*/
  // for(k = 1; k <= N; k++)
  // {
  //   for(j = 1; j <= N; j++)
  //   {
  //     for(i = 1; i <= N; i++)
  //     {
  //       if(dist[i][k] + dist[k][j] < dist[i][j])
  //         dist[i][j] = dist[i][k] + dist[k][j];
  //     }
  //   }
  // }
  /*----------------------------------------------------------------------*/
  /*---------------------- Multi - threaded version ----------------------*/
  for(k = 1; k <= N; k++)//k loop remains same
  {
    pthread_attr_t attr;
    pthread_mutex_init(&mutexdist_rd, NULL);//Initialize mutex variable
    pthread_mutex_init(&mutexdist_wr, NULL);//Initialize mutex variable
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);//default attribute --to create joinable threads
    pthread_t mythreads[N + 1];//instead of i loop we create N threads
    int i;
    for(i = 1; i <= N; i++)
    {
      int retval;
      count_rdrs = 0;
      struct thread_data *data;//Structure to pass parameters to thread
      data = (struct thread_data *)malloc(sizeof(struct thread_data));
      data->k = k;
      data->i = i;
      // printf("In main: creating thread with k = %d, i = %d\n", k, i);
      retval = pthread_create(&mythreads[i], NULL, jloop, (void *)data);//Call j loop from each thread
      if(retval)
      {
        printf("ERROR; return code from pthread_create() is %d\n", retval);
        exit(-1);
      }
    }
    /* ---- Wait for all threads to finish ---- */
    for(i = 1; i <= N; i++)
    {
      int retval;
      void *status;
      retval = pthread_join(mythreads[i], &status);//wait for all threads to finish before starting another iteration
      if (retval)
      {
        printf("ERROR; return code from pthread_join() is %d\n", retval);
        exit(-1);
      }
      //printf("Main: completed join with thread %d having a status of %ld\n",i, (long)status);
    }
    /* ----------------------------------------- */
    pthread_attr_destroy(&attr);//destroy attr
  }
  /*----------------------------------------------------------------------*/
  /*---------------------------- Show output -----------------------------*/
  // printf("Printing Adjacency matrix...\n");
  // for(i = 1; i <= N; i++)
  // {
  //   for(j = 1; j <= N; j++)
  //   {
  //     printf("%d ", Graph[i][j]);
  //   }
  //   printf("\n");
  // }
  printf("Printing dist matrix...\n");
  for(i = 1; i <= N; i++)
  {
    for(j = 1; j <= N; j++)
    {
      if(dist[i][j] == INF)
        printf("INF ");
      else
        printf("%d   ", dist[i][j]);
    }
    printf("\n");
  }
  /*----------------------------------------------------------------------*/
  pthread_mutex_destroy(&mutexdist_wr);//destroy mutex
  pthread_mutex_destroy(&mutexdist_rd);//destroy mutex
  pthread_exit(NULL);
}
