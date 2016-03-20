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
  printf("-----START-----\n");
  int atmid, i = 0;
  atmid = atoi(argv[1]);
  FILE * locfile = fopen("ATMLocator", "rb");
  LOCATION * LocArray[NATMS];
  for(i = 0; i < NATMS; i++)
  {
    LocArray[i] = malloc(sizeof(LOCATION));
    fread(LocArray[i], sizeof(LOCATION), 1, locfile);//Read Locations from ATMLocator file into a local Location array
  }
  fclose(locfile);//Close ATMLocator file
  LOCATION * myLoc = LocArray[atmid];//Get the corresponding entry for the current atm
  key_t key_sem = 222, key_master_msg = 111;
  int clmid, semid, shmid, msmid;
  clmid = msgget(myLoc->key_msg, 0644);//Access Message Queue to communicate with client
  semid = semget(key_sem, NATMS, 0);//Access Semaphores
  shmid = shmget(myLoc->key_shm, SIZE,0666);//Access Local shared memory
  msmid = msgget(key_master_msg, 0644);//Access Message Queue to communicate with Master Process
  printf("ATM #%d started\n", atmid);
  MESSAGE mmsg, cmsg;
  mmsg.pid = atmid;
  mmsg.mtype = ATMMSG;
  void * V = shmat(shmid, NULL, 0);//Attach to Local space
  CLIENT * clientArr = (CLIENT *) (V+(SIZE/2));//Allocate space to store client data
  TRANSACTION * transactionArr = (TRANSACTION *) V;//Allocate space for transaction details
  for(i = 0; i < MAX_NUM_CLIENTS; i++)//Initialize client array in shared memory
  {
    clientArr[i].acnum = -1;
    clientArr[i].balance = 0;
  }
  for(i = 0; i < MAX_NUM_TRANSACTIONS; i++)//Initialize transactions array in shared memory
  {
    transactionArr[i].id = -1;
    transactionArr[i].amount = 0;
  }
  /* ---------------------------- Connection Loop -----------------------------------*/
  while(1)
  {
    printf("Waiting in Connection loop\n");
    msgrcv(clmid, &cmsg, sizeof(cmsg.mtext), CLIENTMSG, 0);//Get message from client process
    int accnum = cmsg.pid;//SEt Account number -- client's process id -- will be used throughout
    printf("ATM %d : Client %d accessed her account\n", atmid, accnum);
    sprintf(mmsg.mtext,"CHECK %d",accnum);
    msgsnd(msmid, &mmsg, sizeof(mmsg.mtext), 0);//Send Check message to master to check for teh existances of the account
    msgrcv(msmid, &mmsg, sizeof(mmsg.mtext), MASTERMSG, 0);
    if(!strcmp(mmsg.mtext,"NEW"))
    {
      printf("Created new Account for client\n");
    }
    else
    {
      printf("Client already has an account\n");
    }
    printf("ATM %d was accessed by Account # %d\n", atmid, accnum);
    cmsg.pid = getpid();
    cmsg.mtype = ATMMSG;
    strcpy(cmsg.mtext,"ACK");
    msgsnd(clmid, &cmsg, sizeof(cmsg.mtext), 0);
    /*-------------------- Transaction loop -----------------------*/
    while(1)
    {
      printf("Waiting in transaction loop\n");
      char command[20];
      msgrcv(clmid, &cmsg, sizeof(cmsg.mtext), CLIENTMSG, 0);
      printf("Command: %s\n", cmsg.mtext);
      if(cmsg.mtext[0] == 'L')
      {
        printf("The Client left. This ATM is now Available for other clients\n");
        break;//Break the transaction loop and wait at the connection loop for other clients
      }
      else if(cmsg.mtext[0] == 'V')
      {
        printf("ATM %d : Requesting a Global Consistency Check\n", atmid);
        sprintf(mmsg.mtext, "VIEW %d", accnum);
        mmsg.mtype = ATMMSG;
        mmsg.pid = getpid();
        int balance = 0;
        msgsnd(msmid, &mmsg, sizeof(mmsg.mtext), 0);//Contact Master Process
        msgrcv(msmid, &mmsg, sizeof(mmsg.mtext), MASTERMSG, 0);//Get Response
        balance += atoi(mmsg.mtext);
        printf("ATM %d : Consistency check returned balance as %d\n", atmid, balance);
        cmsg.mtype = ATMMSG;
        sprintf(cmsg.mtext, "%d", balance);
        msgsnd(clmid, &cmsg, sizeof(cmsg.mtext), 0);//Send balance to client
      }
      else if(cmsg.mtext[0] == 'D')
      {
        int amt;
        sscanf(cmsg.mtext,"%s %d",command,&amt);
        for(i = 0; i < MAX_NUM_TRANSACTIONS; i++)
        {
          if (transactionArr[i].id == -1)//Find a slot in transactionArr and add a new entry
          {
            transactionArr[i].amount = amt;
            transactionArr[i].id = accnum;
            break;
          }
        }
        printf("ATM %d :Deposited %d to Account number %d\n", atmid, amt, accnum);
        cmsg.mtype = ATMMSG;
        sprintf(cmsg.mtext, "SUCCESS");
        msgsnd(clmid, &cmsg, sizeof(cmsg.mtext), 0);//Send success message to client
        continue;
      }
      else if(cmsg.mtext[0] == 'W')
      {
        int amt;
        sscanf(cmsg.mtext,"%s %d", command, &amt);
        printf("ATM %d: Performing Local Consistency Check\n", atmid);
        CLIENT *currClient = NULL;
        int balance = 0;
        printf("Calculated balance: %d\n", balance);
        for(i = 0; i < MAX_NUM_CLIENTS;i++)
        {
          if (clientArr[i].acnum == accnum)//Find the client with the given account number
          {
            printf("Hi\n");
            currClient = &clientArr[i];//Get Client from global memory
            break;
          }
        }
        if(currClient != NULL)
        {
          balance += currClient->balance;//GEt the current balance from global shared memory space
          printf("Calculated balance: %d\n", balance);
        }
        for(i = 0; i < NATMS; i++)//Update balance from transactions corresponding to this client from all ATMs
        {
          int lshmid = shmget(LocArray[i]->key_shm, SIZE, 0666) ;//Access Local shared memory space of all ATMs
          void * V = shmat(lshmid, NULL, 0);//Attach to Local space
          TRANSACTION * ltransactionArr= (TRANSACTION *) V;//Get local transaction Array
          int j = 0;
          for(j = 0; j < MAX_NUM_TRANSACTIONS; j++)
          {
            if(ltransactionArr[j].id == accnum)//GEt all the transactions corresponding to the given account number
            {
              balance += ltransactionArr[j].amount;//Update balance
              printf("Calculated balance: %d\n", balance);
            }
          }
          shmdt(ltransactionArr);
        }
        printf("Calculated balance: %d\n", balance);
        if(balance < amt)//If balance is less than requested amount
        {
          printf("ATM %d : Insufficient funds for this client\n", atmid);
          cmsg.mtype = ATMMSG;
          strcpy(cmsg.mtext, "INSUFFICIENT");
          msgsnd(clmid, &cmsg, sizeof(cmsg.mtext), 0);
        }
        else
        {
          for(i = 0; i < MAX_NUM_TRANSACTIONS; i++)
          {
            if(transactionArr[i].id == -1)//Find an empty slot in transaction array
            {
              transactionArr[i].amount = -1*amt;//negative because it is withdrawal
              transactionArr[i].id = accnum;
              break;
            }
          }
          printf("ATM %d : Successful withdrawal of %d from Account %d\n", atmid, amt, accnum);
          cmsg.mtype = ATMMSG;
          strcpy(cmsg.mtext, "SUCCESS");
          msgsnd(clmid, &cmsg, sizeof(cmsg.mtext), 0);
        }
      }
    }
    /*-------------------------------------------------------------*/
  }
  /* ---------------------------------------------------------------------------------*/
  return 0;
}
