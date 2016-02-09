/* Yeshwanth V   - 13CS10055 */
/* Kshitiz Kumar - 13CS30018 */

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#define MLENGTH 64
#define MAX_NUM_CLIENTS 64
#define CAPACITY 1024
#define SMSG 1L
#define CMSG 2L

typedef struct {
  long mtype;
  long client;
  char mtext[MLENGTH];
}MESSAGE;

typedef struct{
  long pid;
  int index;
}CLIENT;

int main()
{
  key_t key;
  key=ftok(".",'M');
  printf("Hi there! I'm the server\n");
  printf("Main PID is %d\n",getpid());
  CLIENT clientArray[MAX_NUM_CLIENTS];
  int numClients = 0;
  int totNumClients = 0;
  int index = 1;
  int sourceID = 0;
  int msgqID = msgget(key, IPC_CREAT | 0660);
  if(msgqID < 0)
  {
    printf("Error in creating an instance of message queue\n");
    exit(-1);
  }
  while (1)//Keep searching for any incoming requests
  {
    MESSAGE msg;
    if(msgrcv(msgqID, &msg, sizeof(msg.mtext), CMSG, 0) < 0)
    {
      perror("msgrcv error");
      exit(EXIT_FAILURE);
    }
    if(!strcmp(msg.mtext, "couple"))//Couple message from client
    {
      clientArray[totNumClients].pid = msg.client;
      clientArray[totNumClients].index = index;
      printf("Hey a client just coupled: client process id = %ld, client index = %d\n", msg.client, index);
      index++;
      totNumClients++;
      numClients++;
      continue;
    }
    if(!strcmp(msg.mtext, "uncouple"))//Uncouple message from client
    {
      printf("A client uncoupled\n");
      numClients--;
      continue;
    }//
    int i = 0;
    for(i = 0; i < totNumClients; i++)
    {
      if(clientArray[i].pid == msg.client)
      {
        sourceID = clientArray[i].index;
      }
    }
    char temp1[1000];
    char temp2[1000];
    strcpy(temp1,msg.mtext);
    sprintf(temp2,"Terminal # %d :  %s",sourceID,temp1);
    strcpy(msg.mtext,temp2);
    msg.mtype = SMSG;
    for(i = 0; i < numClients; i++)
    {
      if(msgsnd(msgqID, &msg, sizeof(msg.mtext), 0) < 0)
      {
        perror("msgsnd");
        exit(-1);
      }
    }//End for loop
    printf("Message Mirrored\n");
  }//End while loop
}
