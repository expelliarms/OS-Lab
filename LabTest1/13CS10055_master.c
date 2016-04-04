#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#define MAXPARAMS 30
#define DELIM -10798
#define INF 10798

int main(int argc, char const *argv[])
{
  int N, M, i, j, mastermean = 0;
  scanf("%d %d", &N, &M);
  int **Graph;
  Graph = (int **)malloc((N+1) * sizeof(int *));
  for(i = 1; i <= N; i++)
  {
    Graph[i] = (int *)malloc((N+1) * sizeof(int));
    for(j = 1; j <= N; j++)
    {
      Graph[i][j] = 0;
    }
  }
  int **dist = (int **)malloc((N+1) * sizeof(int *));
  for(i = 1; i <= N; i++)
  {
    dist[i] = (int *)malloc((N+1) * sizeof(int));
    for(j = 1; j <= N; j++)
    {
      if(i != j)
        dist[i][j] = INF;
      else
        dist[i][j] = 0;
    }
  }
  int u, v;
  for(i = 1; i <= M; i++)
  {
    scanf("%d %d", &u, &v);
    Graph[u][v] = i;
    Graph[v][u] = -1*i;
    dist[u][v] = 1;
    dist[v][u] = 1;
  }
  int k;
  for(k = 1; k <= N; k++)
  {
    for(j = 1; j <= N; j++)
    {
      for(i = 1; i <= N; i++)
      {
        if(dist[i][k] + dist[k][j] < dist[i][j])
          dist[i][j] = dist[i][k] + dist[k][j];
      }
    }
  }

  int diameter=-1;

  //look for the most distant pair
  for (i = 1; i <= N; ++i){
      for ( j=1;j<=N;++j){
          if (diameter<dist[i][j]){
              diameter=dist[i][j];
              printf("%d %d\n", i, j);
          }
      }
  }
  printf("diameter: %d\n", diameter);
  // printf("Master: Printing Adjacency matrix...\n");
  // for(i = 1; i <= N; i++)
  // {
  //   for(j = 1; j <= N; j++)
  //   {
  //     printf("%d ", Graph[i][j]);
  //   }
  //   printf("\n");
  // }
  srand ( time(NULL) );
  printf("Master: Initial values of nodes -- \n");
  /* Declare pipes */
	int utov[M + 1][2];
	int vtou[M + 1][2];
  for(i = 1; i <= M; i++)
  {
    if(pipe(utov[i]) < 0) perror("Failed to open pipe\n");
    if(pipe(vtou[i]) < 0) perror("Failed to open pipe\n");
  }
  int *outdegree = NULL, *indegree = NULL;
  outdegree = (int *)malloc((N+1) * sizeof(int));
  for(i = 0; i <=N; i++)
  {
    outdegree[i] = 0;
  }
  indegree = (int *)malloc((N+1) * sizeof(int));
  for(i = 0; i <= N; i++)
  {
    indegree[i] = 0;
  }
  // printf("Hi\n" );
  int **outfds, **infds;
  outfds = (int **)malloc((N + 1) * sizeof(int *));
  for(i = 0; i <= N; i++)
  {
    outfds[i] = (int *)malloc(10 * sizeof(int));
  }
  infds = (int **)malloc((N + 1) * sizeof(int *));
  for(i = 0; i <= N; i++)
  {
    infds[i] = (int *)malloc(10 * sizeof(int));
  }
  int childids[N];
  for(i = 1; i <= N; i++)
  {
    for(j = 1; j <= N; j++)
    {
      // printf("i = %d j = %d\n", i, j);
      if(Graph[i][j] > 0)//For outgoing edges
      {
        // printf("Edge ID = %d\n", Graph[i][j]);
        int readEnd = utov[Graph[i][j]][0];
        // printf("readEnd = %d\n", readEnd);
        int writeEnd = utov[Graph[i][j]][1];
        // printf("writeEnd = %d\n", writeEnd);
        outfds[i][outdegree[i]] = writeEnd;
        outdegree[i]++;
        infds[j][indegree[j]] = readEnd;
        indegree[j]++;
      }
      if(Graph[i][j] < 0)//For incoming edges
      {
        // printf("Edge ID = %d\n", Graph[i][j]);
        int writeEnd = vtou[-1*Graph[i][j]][1];
        // printf("writeEnd = %d\n", writeEnd);
        int readEnd = vtou[-1*Graph[i][j]][0];
        // printf("readEnd = %d\n", readEnd);
        infds[j][indegree[j]] = readEnd;
        indegree[j]++;
        outfds[i][outdegree[i]] = writeEnd;
        outdegree[i]++;
      }
    }
  }
  // for(i = 1; i <=N; i++)
  // {
  //   printf("Outfds of %d\n", i);
  //   for(j = 0; j < outdegree[i]; j++)
  //   {
  //     printf("%d ", outfds[i][j]);
  //   }
  //   printf("\n" );
  // }
  // for(i = 1; i <=N; i++)
  // {
  //   printf("infds of %d\n", i);
  //   for(j = 0; j < indegree[i]; j++)
  //   {
  //     printf("%d ", infds[i][j]);
  //   }
  //   printf("\n" );
  // }
  for(i = 1; i <= N; i++)//Create N child processes
  {
    // sleep(3);
    char **parameters;
    parameters = (char **)malloc(MAXPARAMS * sizeof(char *));
    for(j = 0; j < 8; j++)
    {
      parameters[j] = (char *)malloc(50 * sizeof(char));
    }
    sprintf(parameters[0], "./node");
    int nodeval = rand()%100 + 1;
    mastermean += nodeval;
    sprintf(parameters[1], "%d", nodeval);//Set parameter1 to randomly generated node value
    sprintf(parameters[6], "%d", N);//
    sprintf(parameters[7], "%d", diameter);//
    sprintf(parameters[2], "%d", outdegree[i]);//Set parameter2 to degree of the node
    // printf("outdegree = %d\n", outdegree);
    sprintf(parameters[3], "%d", indegree[i]);//Set parameter2 to degree of the node
    // printf("indegree = %d\n", indegree);
    int k;
    for(k = 0; k < outdegree[i]; k++)
    {
      char buf[12];
      sprintf(buf, "%d ", outfds[i][k]); // puts string into buffer
      strcat(parameters[4], buf);
    }
    // printf("Master: parameter 4 : %s\n", parameters[4]);
    for(k = 0; k < indegree[i]; k++)
    {
      char buf[12];
      sprintf(buf, "%d ", infds[i][k]); // puts string into buffer
      strcat(parameters[5], buf);
    }
    // printf("Master: parameter 5 : %s\n", parameters[5]);
    childids[i] = fork();
    if(!childids[i])
    {
      printf("\t(PID: %d, %d)\n", getpid(), nodeval);
      //Child process calls ./node with parameters having generated random number and fds of pipes
      int execret = execvp(parameters[0], parameters);
      if(execret <0 ) perror("Error in exec");
        exit(0);
    }
  }
  mastermean /= N;
  printf("Master: Mean as calculated by master = %d\n", mastermean);
  int childmeans[N];
  for(i = 1; i <= N; i++)
  {
    childmeans[i] = DELIM;
  }
  int alive = N;
  i = 1;
  while (alive > 0)
  {
    if(i == N + 1)
      i = 1;
    if(childmeans[i] == DELIM)
    {
			int status,ret;
			ret = waitpid(childids[i], &status, WNOHANG);
			if(ret == -1)
      {
				perror("waitpid");
				exit(-1);
			}
      else if(ret == childids[i])
      {
				childmeans[i] = WEXITSTATUS(status);
				alive--;
			}
		}
    // sleep(1);
    // printf("i = %d, alive = %d\n", i, alive );
    i++;
  }
  int inconsistent = 0;
  for(i = 1; i <= N; i++)
  {
    int ref = childmeans[i];
    if(ref != mastermean)
    {
      inconsistent = 1;
    }
    for(j = 1; j <= N; j++)
    {
      if(childmeans[j] != ref)
      {
        inconsistent = 1;
        break;
      }
    }
    if(inconsistent == 1)
    {
      break;
    }
  }
  if(inconsistent == 1)
  {
    printf("inconsistent mean calculation\n");
  }
  else
  {
    printf("Mean calculation success: Ans = %d\n", mastermean);
  }
  // sleep(100);
  return 0;
}
