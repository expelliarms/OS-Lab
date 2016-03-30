#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define DELIM -10798
#define INF 10798

int main(int argc, char const *argv[])
{
  if(argc <= 5)
  {
    printf("Invalid args\n");
    exit(-1);
  }
  int i = 0;
  int nodeval = atoi(argv[1]);
  int N = atoi(argv[6]);
  printf("Node %d: nodeval = %d\n", getpid(), nodeval);
  // for(i = 0; argv[i] != NULL; i++)
  // {
  //   printf("Node %d: parameter %d: %s\n",getpid(), i, argv[i]);
  // }
  int degree = atoi(argv[2]);
  int diameter = atoi(argv[7]);
  int writeEnds[degree], readEnds[degree];
  for(i = 0; i < degree; i++)
  {
    sscanf(argv[4], "%d", &writeEnds[i]);
  }
  for(i = 0; i < degree; i++)
  {
    sscanf(argv[5], "%d", &readEnds[i]);
  }
  char *tok = NULL;
  char *savePtr = NULL;
  savePtr = (char *)malloc(20*sizeof(char));
  strcpy(savePtr, argv[4]);
  i = 0;
  while((tok = strtok_r(savePtr, " ", &savePtr)))
  {
    writeEnds[i++] = atoi(tok);
  }
  strcpy(savePtr, argv[5]);
  i = 0;
  while((tok = strtok_r(savePtr, " ", &savePtr)))
  {
    readEnds[i++] = atoi(tok);
  }
  // printf("Node %d: writeEnds: ", getpid());
  // for(i = 0; i < degree; i++)
  // {
  //   printf("%d ", writeEnds[i]);
  // }
  // printf("\n");
  // printf("Node %d: readEnds: ", getpid());
  // for(i = 0; i < degree; i++)
  // {
  //   printf("%d ", readEnds[i]);
  // }
  // printf("\n");



  int mylist[100];
  int listsize = 0;
  int *prevlist;
  prevlist = (int *)malloc(100 * sizeof(int));
  int prevlistcount = 0;
  int numIt;
  if(diameter != INF)
    numIt = diameter;
  else
  {
    numIt = N;
  }
  for(i = 0; i < numIt; i++)
  {
    int *currentlist;
    currentlist = (int *)malloc(100 * sizeof(int));
    int currentlistcount = 0;
    // printf("Node %d: Iteration %d ----------------\n", getpid(), i);
    if(i == 0)//First Iteration
    {
      int k = 0;
      for(k = 0; k < degree; k++)
      {
        if(write(writeEnds[k],&nodeval,sizeof(nodeval)) <= 0)//Every node sends its value to all of its neighbours
        {
          printf("Write Error\n" );
        }
        int delim = DELIM;
        if(write(writeEnds[k],&delim,sizeof(nodeval)) <= 0)
        {
          printf("Write Error\n" );
        }
      }
      for(k = 0; k < degree; k++)
      {
        while(1)
        {
          int read_from_pipe;
          read(readEnds[k],&read_from_pipe,sizeof(read_from_pipe));//Read from the pipe
          // printf("Node %d: readEnds[%d]: %d\n",getpid(), k, read_from_pipe);
          if(read_from_pipe == DELIM)
          {
            break;
          }
          mylist[listsize++] = read_from_pipe;
          currentlist[currentlistcount] = read_from_pipe;
          currentlistcount++;
        }
      }
    }
    else//Subsequent Iterations
    {
      int k = 0;
      for(k = 0; k < degree; k++)
      {
        int m = 0;
        for(m = 0; m < prevlistcount; m++)
        {
          if(write(writeEnds[k],&prevlist[m],sizeof(nodeval)) <= 0)//Every node sends its prev list to all nodes
          {
            printf("Write Error\n" );
          }
        }
        int delim = DELIM;
        if(write(writeEnds[k],&delim,sizeof(nodeval)) <= 0)//Every node sends its value to all of its neighbours
        {
          printf("Write Error\n" );
        }
        while(1)
        {
          int read_from_pipe;
          read(readEnds[k],&read_from_pipe,sizeof(read_from_pipe));//Read from the pipe
          // printf("Node %d: readEnds[%d]: %d\n",getpid(), k, read_from_pipe);
          if(read_from_pipe == DELIM)
          {
            break;
          }
          int newflag = 1;
          for(m = 0; m < listsize; m++)
          {
            if(read_from_pipe == mylist[m])
            {
              newflag = 0;
              break;
            }
          }
          if(newflag == 1)
          {
            mylist[listsize++] = read_from_pipe;
            currentlist[currentlistcount] = read_from_pipe;
            currentlistcount++;
          }
        }
      }
    }
    prevlist = currentlist;
    prevlistcount = currentlistcount;
  }
  int sum = 0;
	for(i = 0; i < listsize; i++)
  {
		sum += mylist[i];
	}
  printf("Node %d: Average = %d\n", getpid(), sum/listsize);
	exit(sum/listsize);
}
