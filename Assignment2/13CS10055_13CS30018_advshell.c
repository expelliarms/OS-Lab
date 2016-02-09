/* Yeshwanth V   - 13CS10055 */
/* Kshitiz Kumar - 13CS30018 */

#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

int simpsh_cd(char **args);
int simpsh_help(char **args);
int simpsh_exit(char **args);
int simpsh_history(char **args);
int line_num,status;
char **history;
FILE * fp,*db;

char *builtin_str[] = 
{
  "cd",
  "help",
  "exit",
  "history"
};

void sig_handler(int sig)
{
  //printf("here\n");
  //stty -echoctl;
  if(!fork())
  {
  history = malloc(1000 *(sizeof(char*)));//Array of Strings to store history
  int i;
  for (i = 0; i < 1000; ++i) 
  {
    history[i] = (char *)malloc(128*sizeof(char));
  }
  line_num = 0;
  ssize_t bufsize = 0;
  fseek(fp,0,SEEK_SET);
  while(getline(&history[line_num], &bufsize, fp)!=-1) line_num++;
  char ch;
  system("/bin/stty raw");
  ch = getchar();
  system("/bin/stty cooked");
  //printf("%c\n",ch);
  for(i=line_num;i>=0;--i)
  {
    if(history[i][0] == ch)
    {
      int j;
      int siz = strlen(history[i]);
      for(j=1;j<siz;++j)
        printf("%c",history[i][j]);
      break;
    }
  }
}
else
{
  wait(&status);
}
}
void simpsh_redirecting()
{
    
}
int (*builtin_func[]) (char **) = 
{
  &simpsh_cd,
  &simpsh_help,
  &simpsh_exit,
  &simpsh_history
};

int simpsh_num_builtins() 
{
  return sizeof(builtin_str) / sizeof(char *);
}

int simpsh_cd(char **args)
{
  if (args[1] == NULL) 
  {
    fprintf(stderr, "simpsh: expected argument to \"cd\"\n");
  } 
  else 
  {
    if (chdir(args[1]) != 0) {
      perror("simpsh");
    }
  }
  return 1;
}

int simpsh_help(char **args)
{
  int i;
  printf("Type program names and arguments, and hit enter.\n");
  printf("The following are built in:\n");
  for (i = 0; i < simpsh_num_builtins(); i++) 
  {
    printf("  %s\n", builtin_str[i]);
  }
  printf("Use the man command for information on other programs.\n");
  return 1;
}

/* Function to exit shell */
int simpsh_exit(char **args)
{
  return 0;
}

/* Function to print the history */
int simpsh_history(char **args)
{
  history = malloc(1000 *(sizeof(char*)));//Array of Strings to store history
  int i;
  for (i = 0; i < 1000; ++i) 
  {
    history[i] = (char *)malloc(128*sizeof(char));
  }
  line_num = 0;
  ssize_t bufsize = 0;
  fseek(fp,0,SEEK_SET);
  while(getline(&history[line_num], &bufsize, fp)!=-1) line_num++;//Read history from the file fp
  if(!strcmp(args[0],"history"))//Display command history
  {
    if(args[1]==NULL)//Complete history
    {
      printf("\nDisplaying complete command history\n\n");
      int i=0;
      for(i=0;i<line_num;i++) printf("%s",history[i]);//Loop through the history array and print all the commands
      putchar('\n');  
    }
    else//Partial History 
    {
      int argnum=atoi(args[1]);
      int i=0;
      printf("\nDisplaying partial command history\n\n");
      for(i=line_num-argnum;i<line_num;i++) printf("%s",history[i]);
      putchar('\n');  
    } 
  }
}
/**
  Function to Launch a program and wait for it to terminate.
  Return status -- Always returns 1, to continue execution.
 */
int simpsh_launch(char **args)
{
  if(args[0][strlen(args[0])-1] == '&')
  {
    args[0][strlen(args[0])-1] = '\0';
    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0)//Child process
    {
      if (execvp(args[0], args) == -1)
      {
        perror("shell error");
      }
      setpgid(0, 0);
      exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
      perror("shell error");
    }
    else//Parent process
    {
      if (!tcsetpgrp(STDIN_FILENO, getpid()))
      {
        perror("tcsetpgrp failed");
      }
    }
  }
  else
  {
    pid_t pid, wpid;
    int status;
    pid = fork();
    if (pid == 0) 
    {
      int pipeline[50];//Variables to maintain the Number of Commands --Pipeline
      int num_pipes=0;
      pipeline[0]=0;      
      int i=0,j=1;
      while(args[i]!=NULL)
      {
        if(!strcmp(args[i],"|"))//Check for the Pipe symbol         
        {
          args[i]=NULL;
          pipeline[j]=i+1;//Initialize the indexes for each pipes
          j++;
          num_pipes++;//Increment number of pipes
        }                     
        i++;
      }     
      int num_commands=num_pipes+1;
      int pipes[num_pipes][2];//Array of Pipe file-descriptors
      for(i=0;i<num_pipes;i++)//Create num_pipes number of pipes
      {
        if(pipe(pipes[i])<0) 
        {
          perror("Error while creating pipe\n");
          exit(0);
        }
      }
      int command_start;
      //fflush(0);
      //fprintf(db,"num_pipes = %d\n",num_pipes);
      //fflush(db);
      int p=0;
      for(i=0;i<num_commands;i++)//Loop num_commands times
      {   
        command_start=pipeline[i]; 
        //fprintf(db,"command_start=%d\n",command_start);
        //fflush(db);
        pid=fork();
        if(pid==0)
        { 
          //fflush(0);
          //fprintf(db,"Child PID is %d\n",getpid());
          if(i<num_pipes)      //Do this for all pipes except the last -- because the last pipe should output into the std out
          {        
            if(dup2(pipes[(p+1)/2][1],STDOUT_FILENO)<0)//Close the existing file descriptor(file descriptor of std out) and create a new one
            {
              perror("dup error\n");
              exit(0);           
            }
            //close(pipes[(int)i/2][1]);
            //fprintf(db,"In 1 and pipe # = %d\n",(p+1)/2);
          }
          if(i!=0)//Do this for all pipes except the first -- because the first pipe should take input from std in
          {
            if(dup2(pipes[(p-2)/2][0],STDIN_FILENO)<0)//Change the file descriptor from std in to pipe's read end
            {
              perror("dup error\n");
              exit(0);
            }
            //close(pipes[(int)i/2][0]);
            //fprintf(db,"In 2 and pipe # = %d\n",(p-2)/2);  
          }
          //fflush(db); 
          for( i= 0; i < num_pipes; i++)
          {
            close(pipes[i][0]);
            close(pipes[i][1]);
          }    

          //-------------------------------           
          int x=0;
          int arg_count=0;
          char infile[100];
          int has_infile=0;
          int has_outfile=0;
          char outfile[100];
          while(1)
          {
            if(args[arg_count]!=NULL) arg_count++;
            else break;
          }
          for(x=0;x<arg_count;x++)
          {  
            if(args[x]!=NULL)
            {
              if(!strcmp(args[x],">"))
              {
                strcpy(outfile,args[x+1]);
                has_outfile=1;//Set has output file flag to 1
                args[x]=NULL;
              }
            }
            if(args[x]!=NULL)
            {
              if(!strcmp(args[x],"<"))
              {
                strcpy(infile,args[x+1]);
                has_infile=1;// Srt has input file flag to 1
                args[x]=NULL;
              }
            }
          }  
          if(has_infile)        //Redirect Input
          { 
            int fdI = open(infile, O_RDONLY);//Open the input file in readonly mode
            dup2(fdI, STDIN_FILENO);//Change the file descriptor so that the input is taken from the file specified in the command instead of stdin
            close(fdI);               
          }
          if(has_outfile)       //Redirect Output
          {
              int fdO = creat(outfile, 0644);//Create output file in write mode
            dup2(fdO, STDOUT_FILENO);// Change the file descriptor so that the output us written into the specified file instead of std out
            close(fdO);
          }
          if(execvp(args[command_start],args+command_start) <0 ) perror ("Execution failed\n");
            exit(0);
        } 
        else if (pid <0) 
        {
          perror("Error creating process\n");
          exit(0);
        } 
        p+=2;
      }//End Pipe loop     
      for(i = 0; i < num_pipes; i++)
      {
        close(pipes[i][0]);
        close(pipes[i][1]);
      }

      exit(EXIT_FAILURE);
    }
    else if (pid < 0)
    {
      perror("lsh");
    }
    else  
    {
      do
      {
        wpid = waitpid(pid, &status, WUNTRACED);
      }
      while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
  }
  return 1;
}

/**
   Function to Execute shell built-in or launch program.
   Parameters: args -- Null terminated list of arguments.
   Return: status - 1 if the shell should continue running, 0 if it should terminate
 */
int simpsh_execute(char **args)
{
  int i;
  if (args[0] == NULL) 
  {
    return 1;
  }
  for (i = 0; i < simpsh_num_builtins(); i++) 
  {
    if (strcmp(args[0], builtin_str[i]) == 0) 
    {
      return (*builtin_func[i])(args);
    }
  }
  return simpsh_launch(args);
}

#define LSH_RL_BUFSIZE 1024
/**
   Function to Read a line of input from stdin.
   Returns The line from stdin.
*/     
char *simpsh_read_line(void)
{
  line_num++;
  int bufsize = LSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;
  if (!buffer) 
  {
    fprintf(stderr, "simpsh: allocation error\n");
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
      fseek(fp,0,SEEK_END);
      fprintf(fp,"%s\n",buffer);
      fflush(fp);  
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
      bufsize += LSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) 
      {
        fprintf(stderr, "simpsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a"
/*
   Function to Split a line into tokens
   Paramenters -- line: input line.
   return value -- Null-terminated array of tokens.
*/
char **simpsh_split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;
  if (!tokens) 
  {
    fprintf(stderr, "simpsh: allocation error\n");
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
        fprintf(stderr, "simpsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/* Loop for getting input and executing it. */
void simpsh_loop(void)
{
  char *line;
  char **args;
  int status;
  char cwd[1024];
  do {
    getcwd(cwd, sizeof(cwd));
    printf("%s > ",cwd);
    line = simpsh_read_line();//Call function to read input line
    args = simpsh_split_line(line);//Call function to split the input line into arguments
    status = simpsh_execute(args);//Call function to execute the command
    free(line);//Free variables
    free(args);
  } while (status);//Run the loop continiously as long as the status is non zero
}

/* main function */
int main(int argc, char **argv,char** envp)
{
  signal(SIGQUIT,sig_handler);
  fp=fopen("./sh_log","w+"); //Log file for storing history
  db=fopen("./debug_log","w+"); 
  char ** args;
  char *input ;
  int i=0;
  simpsh_loop();
  return EXIT_SUCCESS;
}