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
#define CLIENTMSG 1
#define ATMMSG 2

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

int main(int argc, char const *argv[])
{
  FILE * locfile = fopen("ATMLocator","rb");
  LOCATION * LocArray[NATMS];
  int i = 0, j;
  struct sembuf *sops = (struct sembuf *) malloc(2*sizeof(struct sembuf));
  int nsops = 0;// number of operations to do
  for(i = 0; i < NATMS; i++)
  {
    LocArray[i] = malloc(sizeof(LOCATION));
    fread(LocArray[i], sizeof(LOCATION), 1, locfile);//Read Locations from the file into a local array
  }
  fclose(locfile);
  key_t key_sem = 222;
  int semid, mid;

  /* ------------------------------- Command Loop -----------------------------------*/
  while (1)
  {
    printf("Welcome to the ATM System . Available Commands :\n 1) ENTER <ATM ID> \n 2) WITHDRAW <Amount>\n 3) DEPOSIT <Amount>\n 4) VIEW\n 5) LEAVE\n");
    char command[20];
    int atmid;
    printf("\n>>> ");
    scanf("%s %d",command,&atmid);
    if((strcmp(command,"ENTER"))||(atmid >= NATMS))
    {
      printf("Invalid command\n");
      continue;//Continue until a proper enter command
    }
    LOCATION *myLoc = LocArray[atmid];//Get the location array entry corresponding to the atmid
    mid = msgget(myLoc->key_msg, 0644);//Access Message Queue to communicate with atm process
    semid = semget(key_sem, NATMS, 0);//Access Semaphores
    int status = semctl(semid, atmid, GETVAL, 0);//GEt semaphore status
    if(status != 0)
    {
      printf("ATM is occupied\n");
      continue;
    }
    printf("Waiting for semaphore[%d]\n", atmid);
    nsops = 2;//Number of operations to do
    sops[0].sem_num = atmid;
    sops[0].sem_op = 0;//Wait for the semaphore to become zero
    sops[0].sem_flg = 0;
    sops[1].sem_num = atmid;
    sops[1].sem_op = 1;//Take control of the semaphore
    sops[1].sem_flg = 0;
    if ((j = semop(semid, sops, nsops)) == -1)
    {
      printf("semop: semop failed\n");
      exit(1);
    }
    else
    {
      printf("Process Acquired semaphore[%d]\n", atmid);
      MESSAGE msg1;
      msg1.pid = getpid();
      printf("Process id: %ld\n", msg1.pid);
      printf("You can access your account now!\n");
      msg1.mtype = CLIENTMSG;
      strcpy(msg1.mtext,"ENTER");
      msgsnd(mid, &msg1, sizeof(msg1.mtext), 0);
      // printf("Semaphore state Now\n");
      // for(i = 0; i<NATMS; i++)
      // {
      //   int retval=semctl(semid, i, GETVAL, 0);
      //   printf("semaphore[%d] = %d\n", i, retval);
      // }
      msgrcv(mid, &msg1, sizeof(msg1.mtext), ATMMSG, 0);
      printf("Ack message from atm: %s\n", msg1.mtext);
      if(strcmp(msg1.mtext,"ACK") != 0)
      {
        printf("Some error occured in entering this ATM. Please try again...\n");
        continue;
      }
      /* ---------------- INNER COMMAND LOOP ----------------- */
      while (1)
      {
        printf("\n\t>>> ");
        scanf("%s", command);
        if(!strcmp(command,"LEAVE"))
        {
          printf("Exiting...\n");
          nsops = 1;//Number of operations to do
          sops[0].sem_num = atmid;
          sops[0].sem_op = -1;//Give UP COntrol of Semaphore
          sops[0].sem_flg = 0;
          if ((j = semop(semid, sops, nsops)) == -1)
          {
            perror("semop: semop failed");
          }
          else
          {
            printf("Process Giving up Control of semaphore\n");
            printf("Semaphore state Now\n");
            for(i = 0; i<NATMS; i++)
            {
              int retval=semctl(semid, i, GETVAL, 0);
              printf("semaphore[%d] = %d\n", i, retval);
            }
            msg1.pid = getpid();
            strcpy(msg1.mtext,"LEAVE");
            msg1.mtype = CLIENTMSG;
            msgsnd(mid, &msg1, sizeof(msg1.mtext), 0);
          }
          break;
        }
        else if(!strcmp(command,"VIEW"))
        {
          strcpy(msg1.mtext,"VIEW");
          msg1.mtype = CLIENTMSG;
          msgsnd(mid, &msg1, sizeof(msg1.mtext), 0);
          msgrcv(mid, &msg1, sizeof(msg1.mtext), ATMMSG, 0);
          int balance = atoi(msg1.mtext);
          printf("Your current balance is %d\n", balance);
          continue;
        }
        else if(!strcmp(command,"DEPOSIT"))
        {
          int amt;
          scanf("%d",&amt);
          // printf("%d\n", amt);
          if(amt <= 0)
          {
            printf("Invalid Amount\n");
            continue;
          }
          sprintf(msg1.mtext, "DEPOSIT %d", amt);
          msg1.mtype = CLIENTMSG;
          msgsnd(mid, &msg1, sizeof(msg1.mtext), 0);//Send command to atm process
          msgrcv(mid, &msg1, sizeof(msg1.mtext), ATMMSG, 0);//Get Response
          printf("%s\n", msg1.mtext);
          if(!strcmp(msg1.mtext,"SUCCESS"))
          {
            printf("Transaction successful.\n");
          }
          else
          {
            printf("There was an error in your transaction\n");
          }
          continue;
        }
        else if(!strcmp(command,"WITHDRAW"))
        {
          int amt;
          scanf("%d",&amt);
          sprintf(msg1.mtext,"WITHDRAW %d",amt);
          if(amt <= 0)
          {
            printf("Invalid Amount\n");
            continue;
          }
          msg1.mtype = CLIENTMSG;
          msgsnd(mid, &msg1, sizeof(msg1.mtext), 0);//Send command to atm process
          msgrcv(mid, &msg1, sizeof(msg1.mtext), ATMMSG, 0);//Get Response
          printf("%s\n", msg1.mtext);
          if(!strcmp(msg1.mtext,"SUCCESS"))
          {
            printf("Transaction successful!\n");
          }
          else if(!strcmp(msg1.mtext,"INSUFFICIENT"))
          {
            printf("Transaction failed: Insufficient funds.\n");
          }
          else
          {
            printf("There was an error in your transaction\n");
          }
          continue;
        }
        else
        {
          printf("Invalid Command, Try again...\n");
        }
      }
      /* ----------------------------------------------------- */
    }
  }
  /* ---------------------------------------------------------------------------------*/

  return 0;
}
