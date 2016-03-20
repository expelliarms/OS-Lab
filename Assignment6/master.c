/*
Yeshwanth -- 13CS10055
Kshitiz Kumar -- 13CS30018
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <math.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define NATMS 5
#define SIZE 16384
#define CLIENTMSG 1
#define ATMMSG 2
#define MASTERMSG 3
#define MAX_NUM_CLIENTS 10
#define MAX_NUM_TRANSACTIONS 50

typedef struct location{
  int id;
  int key_msg;
  int key_shm;
  int pid;
}LOCATION;

typedef struct message{
	long mtype;
  long pid;
	char mtext[500];
}MESSAGE;

typedef struct client{
  int acnum;
  int balance;
}CLIENT;

typedef struct transaction{
  int id;
  int amount;
}TRANSACTION;

int main(int argc, char const *argv[])
{
  printf("-------START-------\n");
  key_t key_msg, key_sem, key_shm;
  int mid, semid, shmid;
  key_msg = 111;
  key_sem = 222;
  key_shm = 333;
  FILE * locfile = fopen("ATMLocator", "wb");

  /* ------- Delete all previous existances of the Shared resources (if any) --------*/
  if((mid = msgget(key_msg,0644 | IPC_CREAT)) == -1)//Get Message Queue
  {
    printf("msgget error\n");
    exit(1);
  }
  if(msgctl(mid, IPC_RMID, NULL) == -1)//Delete Message Queue
  {
    printf("msgctl IPC_RMID error\n");
    exit(1);
  }
  if((semid = semget(key_sem, NATMS, IPC_CREAT | 0666)) == -1)//Get NATMS Semaphores
  {
    printf("semget error\n");
    exit(1);
  }
  if(semctl(semid, 0, IPC_RMID) == -1)//Delete Semaphores
  {
    printf("semctl IPC_RMID Error\n");
    exit(1);
  }
  if((shmid = shmget(key_shm, SIZE, IPC_CREAT | 0666)) == -1)//Get Global Shared Memory
  {
    printf("shmget error\n");
    exit(1);
  }
  if(shmctl(shmid, 0, IPC_RMID) == -1)//Delete Global Shared Memory
  {
    printf("shmctl IPC_RMID Error\n");
    exit(1);
  }
  int i = 0;
  for(i = 0; i < NATMS; i++)//Delete Local Shared Memories
  {
    int lshmid, lmid;
    key_t key_lshm, key_lmsg;
    key_lshm = i*i + 100;
    key_lmsg = i*i + 500;
    if((lshmid = shmget(key_lshm, SIZE, IPC_CREAT | 0666)) == -1)//Get Local Shared Memories
    {
      printf("shmget error\n");
      exit(1);
    }
    shmctl(lshmid, IPC_RMID, NULL);//Delete Local Shared Memory
    if(lmid = msgget(key_lmsg, 0644 | IPC_CREAT) == -1)//Get local Message Queues
    {
      printf("msgget error\n");
      exit(1);
    }
    msgctl(lmid, IPC_RMID, NULL);//Delete Local Message Queues
  }
  /* ---------------------------------------------------------------------------------*/

  /*-------------------------- Create New Shared Resources ---------------------------*/
  if ((semid = semget(key_sem, NATMS, IPC_CREAT | 0666)) == -1)//Create Semaphores -- One for each atm
  {
    printf("semget failed\n");
    exit(1);
  }
  for(i = 0; i < NATMS; i++)//Initializing all (sub)semaphores to 0
  {
    semctl(semid, i, SETVAL, 0);
    int retval = semctl(semid, i, GETVAL, 0);
    printf("(Sub)Semaphore #%d: Value = %d\n",i, retval);
  }
  if((mid = msgget(key_msg, 0644 | IPC_CREAT)) == -1)//Create Message Queue
  {
    perror("msgget");
    exit(1);
  }
  if ((shmid = shmget (key_shm, SIZE, IPC_CREAT | 0666)) == -1)//Create Global Shared Memory
  {
    perror("shmget: shmget failed"); exit(1);
  }
  LOCATION * LocArray[NATMS];
  for(i = 0; i < NATMS; i++)//Create Local shared Memory for each atm
  {
    int lshmid, lmid;
    key_t key_lshm, key_lmsg;
    key_lshm = i*i + 100;
    key_lmsg = i*i + 500;
    if((lshmid = shmget(key_lshm, SIZE, IPC_CREAT | 0666)) == -1)//Get Local Shared Memories
    {
      printf("shmget error\n");
      exit(1);
    }
    if(lmid = msgget(key_lmsg, 0644 | IPC_CREAT) == -1)//Get local Message Queues
    {
      printf("msgget error\n");
      exit(1);
    }
    LOCATION * l = malloc(sizeof(LOCATION));//A location Struct
    l->id = i;
    l->key_msg = key_lmsg;
    l->key_shm = key_lshm;
    l->pid = 0;
    fwrite(l, sizeof(LOCATION), 1, locfile);//Write the struct to file
    LocArray[i]=l;
  }
  fclose(locfile);//Close ATMLocator file
  /* ---------------------------------------------------------------------------------*/

  /* -------------------------- Create ATM processes ---------------------------------*/
  for(i = 0; i < NATMS; i++)
  {
    pid_t pid;
    char parameter1[2];
    printf("Creating ATM # %d\n",i);
    if((pid = fork()) < 0)
    {
      perror("fork");
      exit(1);
    }
    if(pid == 0)
    {
      sprintf(parameter1, "%d", i);
      execlp("xterm","xterm","-hold","-e","./atm",parameter1,(const char *) NULL);
    }
    else
      LocArray[i]->pid = pid;
  }
  /* ---------------------------------------------------------------------------------*/

  /* ---------------------------- Initialize clientArr -------------------------------*/
  CLIENT * clientArr = (CLIENT *) shmat(shmid, NULL, 0);
  for(i = 0; i < MAX_NUM_CLIENTS; i++)
  {
      clientArr[i].acnum = -1;
      clientArr[i].balance = 0;
  }
  /* ---------------------------------------------------------------------------------*/

  /* ----------------------------- Handle messages -----------------------------------*/
  MESSAGE amsg;
  char command[20];
  while(1)
  {
    msgrcv(mid, &amsg, sizeof(amsg.mtext), ATMMSG, 0);
    if(amsg.mtext[0] == 'C')
    {
      printf("Received a Check account request\n");
      int accnum = 0;
      sscanf(amsg.mtext, "%s %d", command, &accnum);
      int existing = 0;
      for(i = 0; i < MAX_NUM_CLIENTS;i++)
      {
        if (clientArr[i].acnum == accnum)//There is a client with the given account number
        {
          printf("Account Exists\n");
          existing = 1;
          amsg.pid = getpid();
          strcpy(amsg.mtext,"EXISTING");
          amsg.mtype = MASTERMSG;
          msgsnd(mid, &amsg, sizeof(amsg.mtext), 0);//SEnd a message to ATM process telling that the account queried exists
          break;
        }
      }
      if(existing == 0)//There is no entry in clients array
      {
        for(i = 0; i < MAX_NUM_CLIENTS; i++)
        {
          if (clientArr[i].acnum == -1)//Find a empty account slot
          {
            printf("New Account Created\n");
            clientArr[i].acnum = accnum;//Assign account number
            clientArr[i].balance = 0;//Assign zero balance to new account
            amsg.pid = getpid();
            strcpy(amsg.mtext, "NEW");
            amsg.mtype = MASTERMSG;
            msgsnd(mid, &amsg, sizeof(amsg.mtext), 0);
            break;
          }
        }
      }
    }
    else if(amsg.mtext[0] == 'V')//Global Consistency Check
    {
      int accnum;
      printf("MANAGER : Performing a global consistency check\n");
      sscanf(amsg.mtext, "%s %d", command, &accnum);
      CLIENT *currClient;
      for(i = 0; i < MAX_NUM_CLIENTS;i++)
      {
        if (clientArr[i].acnum == accnum)//Find the client with the given account number
        {
          currClient = &clientArr[i];//Get Client from global memory
          break;
        }
      }
      int balance = currClient->balance;//Get balance from global address space
      for(i = 0; i < NATMS; i++)//Get balance form all ATMs
      {
        int lshmid = shmget(LocArray[i]->key_shm, SIZE, 0666) ;//Access Local shared memory of other ATMs
        void * V = shmat(lshmid, NULL, 0);//Attach to Local space
        CLIENT * lclientArr = (CLIENT *) (V + SIZE/2);//Partion of memory for client array and Transaction array
        TRANSACTION * ltransactionArr = (TRANSACTION *) V;//Get transactionArr in local space
        int j = 0;
        for(j = 0; j < MAX_NUM_TRANSACTIONS; j++)
        {
          if(ltransactionArr[j].id == accnum)//Find transactions of the give client
          {
            balance += (ltransactionArr[j].amount);//Update balance in local space
            ltransactionArr[j].amount = 0;
            ltransactionArr[j].id = -1;//Clear the Transaction
          }
        }
        currClient->balance = balance;//Update global record of balance
        CLIENT * lcurrClient = NULL;
        for(j = 0; j < MAX_NUM_CLIENTS; j++)
        {
          if (lclientArr[j].acnum == accnum)//Find the client with the given account number in local client array
          {
            lcurrClient = &lclientArr[i];//Get Client from local memory
            break;
          }
        }
        if(lcurrClient == NULL)//If there is no entry corresponding to this client in the local address space of this ATM
        {
          for(j = 0; j < MAX_NUM_CLIENTS; j++)//Find an empty slot
          {
            if(lclientArr[j].acnum == -1)
              break;
          }
          lclientArr[j].acnum = accnum;//Add entry
          lcurrClient = &(lclientArr[j]);
        }
        lcurrClient->balance = balance;//Update Local Images with Account balance
        shmdt(V);
      }
      sprintf(amsg.mtext,"%d", balance);
      amsg.mtype = MASTERMSG;
      msgsnd(mid, &amsg, sizeof(amsg.mtext), 0);//Return Balance
    }
  }
  /* ---------------------------------------------------------------------------------*/
  return 0;
}
