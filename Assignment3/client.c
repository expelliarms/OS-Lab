/* Yeshwanth V   - 13CS10055 */
/* Kshitiz Kumar - 13CS30018 */

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#define MLENGTH 50
#define CAPACITY 1000
#define SMSG 1L
#define CMSG 2L
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"

typedef struct {
  long mtype;
  long client;
  char mtext[MLENGTH];
}MESSAGE;

char *simpsh_read_line(void)
{
  //line_num++;
  int bufsize = CAPACITY;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;
  if (!buffer)
  {
    printf("simpsh: allocation error\n");
    exit(EXIT_FAILURE);
  }
  while (1)
  {
    // Read a character
    c = getchar();
    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n')
    {
      buffer[position] = '\0';
      //fseek(fp,0,SEEK_END);
      //fprintf(fp,"%s\n",buffer);
      //fflush(fp);
      return buffer;
    }
    else
    {
      buffer[position] = c;
    }
    position++;
    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize)
    {
      bufsize += CAPACITY;
      buffer = realloc(buffer, bufsize);
      if (!buffer)
      {
        printf("simpsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

char **simpsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;
  if (!tokens)
  {
    printf("simpsh: allocation error\n");
    exit(EXIT_FAILURE);
  }
  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;
    if (position >= bufsize)
    {
      bufsize += LSH_TOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens)
      {
		    free(tokens_backup);
        printf("simpsh: re allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

int main()
{
  char buffer[CAPACITY];
  printf("Hi this a client\n" );
  printf("Main PID is %d\n",getpid());
  int coupled_flag = 0;
  pid_t ch_pid;
  MESSAGE msg;
  int msgqID;
  key_t key;
  while (1)
  {
    char *line;
    char **args;
    getcwd( buffer,100);
    printf("%s >> ", buffer);
    line = simpsh_read_line();//Call function to read input line
    args = simpsh_split_line(line);//Call function to split the input line into arguments
    //printf("%s\n", line);
    if(!strcmp(args[0],"exit"))
    {
      kill(ch_pid,SIGKILL);	//Kill the child process
      coupled_flag = 0;//Reset the coupled flag
      msg.mtype = CMSG;
      msg.client = (long) getpid();
      strcpy(msg.mtext, "uncouple");
      if(msgsnd(msgqID, &msg, sizeof(msg.mtext), 0) < 0)//Send uncouple command to the Server
      {
        perror("msgsnd");
        exit(-1);
      }
      break;//Break the infinite while loop
    }//End exit comman
    if(!strcmp(args[0],"uncouple"))
    {
      kill(ch_pid,SIGKILL);	//Kill the child process
      coupled_flag = 0;//Reset the coupled flag
      msg.mtype = CMSG;
      msg.client = (long) getpid();
      strcpy(msg.mtext, "uncouple");
      if(msgsnd(msgqID, &msg, sizeof(msg.mtext), 0) < 0)//Send uncouple command to the Server
      {
        perror("msgsnd");
        exit(-1);
      }
      continue;//Break the infinite while loop
    }//End uncouple command
    if(!strcmp(args[0],"couple"))//Couple Client and Server
    {
      key = ftok(".", 'M');//convert pathname and project identifier to a System V IPC key
      msgqID = msgget(key, 0);
      coupled_flag = 1;//Set the coupletd flag to 1
      msg.mtype = CMSG;
      msg.client = (long) getpid();
      strcpy(msg.mtext, "couple");//Copy message text into the message struct
      //printf("Hi 1\n");
      if(msgsnd(msgqID, &msg, sizeof(msg.mtext), 0) < 0)//Send couple command to the Server
      {
        perror("msgsnd");
        exit(-1);
      }
      //("Hi 2\n" );
      if(!(ch_pid=fork()))//A child process to handle mirroring
      {
        while(1)//Keep checking for any incoming messages
        {
          MESSAGE recv_msg;
          //printf("Hi 1\n");
          if(msgrcv(msgqID, &recv_msg, sizeof(recv_msg.mtext), SMSG , 0)<0)
          {
            perror("msgrcv");
            exit(-1);
          }
          //("Hi 2\n" );
          if( (pid_t) recv_msg.client != getppid() )//Check if the terminal is mirroring itserlf
          {
            printf("\n%s",recv_msg.mtext);
          }
        }
      }
      else
        perror("fork");
      printf("Coupled\n");
      printf("Child PID is %d \n",ch_pid);
      continue;
    }//End couple command
    char * out;
    FILE * f = popen(line,"r");	//Use popen to create a pipe between calling process and shell output
    //opens  a process by creating a pipe, forking, and invoking the shell.  Since a pipe is by definition unidirectional, "r" argument is given to open pipe in read only mode
    if(!f)
      perror("popen error\n");
    else
    {
      fseek(f, 0, SEEK_END);
      long fsize = ftell(f);
      rewind(f);
      out = malloc(fsize + 1);
      fread(out, fsize, 1, f);
      out[fsize] = '\0';
    }
    printf("%s\n",out);
    sprintf(msg.mtext,"%s%s",line,out);
    pclose(f);
    if(coupled_flag)
    {
      //printf("Coupled flag is set to 1\n");
      msg.mtype = CMSG;
      msg.client = (long) getpid();
      if(msgsnd(msgqID, &msg, sizeof(msg.mtext)))
      {
        perror("msgsnd error");
        exit(EXIT_FAILURE);
      }
    }
  }//End while loop
  return 0;
}
