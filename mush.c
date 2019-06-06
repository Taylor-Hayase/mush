/*mush written by Taylor Hayase and Ryan Premi*/

#define _BSD_SOURCE
#define _POSIX_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#include "parseline.h"

static int boo = 0;

void handler(int pid)
{
   printf("\n");
   if (!boo)
      exit(0);
   boo = 0;
   kill(pid, SIGTERM);
}

int main(int argc, char *argv[])
{
	Stage *head, *ptr; 
	int i=0, num_pipes = 0, child_num;
	int pid, childStatus;
	int fd[2];
   int read_pipe[2];
   int write_pipe[2];
   int file;
   int filein = 1, fild;
	char **args;
   char buff[CMDL_MAX + 1];
	args = malloc((10) *sizeof(char *));

   signal(SIGINT, handler);

   while(1)
   {
      num_pipes = 0;
      boo = 0;
      if (argc == 1 || !filein)
   	   printf("8-P ");

      /*if a file with args is given*/
      if (filein && (argc > 1))
      {

         if((fild = open(argv[1], 0666)) == -1)
         {
            fprintf(stderr, "file open failed\n");
            continue;
         }
         if (read(fild, buff, CMDL_MAX) == -1)
         {
            fprintf(stderr, "file read failed\n");
            continue;
         }
         close(fild);

         head = parse_main(argc, buff);

         filein = 0;
      }
      else /*otherwise treat normally*/
   	   head = parse_main(1, argv[0]);

	   if (head == NULL)
		   continue;

      /*process that cannot be run in a child: cd*/
      if (strcmp(head->argv[0], "cd") == 0)
      {
         boo = 1;
         if (chdir(head->argv[1]) == -1)
         {
            fprintf(stderr, "Not a directory\n");
            continue;
         }
         continue;
      }

      /*base case*/
      if(head->next == NULL)
      {
         boo = 1;
         /*convert for execvp*/
   	   for (i = 0; i < head->argc; i++)
		      args[i] =head->argv[i];

	      args[i] = NULL;

   	   if ((pid =fork()) == 0) /*child*/
         {
            /*change if not standard in*/
   	 	   if(strcmp("stdin", head->input) != 0)
            {
   		      fd[0] = open(head->input, 0666);
		         dup2(fd[0], 0);
			      close(fd[0]);
   	 	   }
            /*change if not stantard out*/
   	 	   if(strcmp("stdout", head->output) != 0)
            {
   	 	  	   fd[1] = creat(head->output, 0666);
			      dup2(fd[1], 1);
			      dup2(fd[1], 2);
			      close(fd[1]);
    	      }
	 	      execvp(args[0], args);
   	   }
   	   else /*parent*/
   	 	   waitpid(pid, &childStatus, 0);

         continue;
      }

      ptr = head;

      if (pipe(read_pipe) == -1)
      {
         perror("pipe");
         exit(1);
      }
      /*get length of stages*/
      while (ptr != NULL)
      {
         num_pipes++;
         ptr = ptr->next;
      }
 
      ptr = head;


      for (child_num = 0; child_num < num_pipes; child_num++)
      {
         if (pipe(write_pipe) == -1)
         {
            perror("pipe");
            exit(1);
         }

         pid = fork();

         /*if parent*/
         if (pid != 0)
         {
            if (pid < 0)
            {
               perror("fork");
               exit(1);
            }

            close(read_pipe[1]);
            wait(NULL);

            read_pipe[0] = write_pipe[0];
            read_pipe[1] = write_pipe[1];

         }
         else
         {
            boo = 1;
            for (i = 0; i < ptr->argc; i++)
		         args[i] =ptr->argv[i];
	         
	         args[i] = NULL;

            /*read stdin from pipe*/
   	 	   if(strcmp("stdin", ptr->input) != 0)
		         dup2(read_pipe[0], 0);
   	 	   
            /*write stdout to pipe*/

   	 	   if(strcmp("stdout", ptr->output) != 0)
            {
               if (child_num == num_pipes - 1)
               {
   	 	  	      file = creat(ptr->output, 0666);
			         dup2(file, 1);
			         dup2(file, 2);
			         close(file);
               }
               else 
               {
			         dup2(write_pipe[1], 1);
                  dup2(write_pipe[1], 2);
               }
   	 	   }
            else
            {
               if (child_num == num_pipes - 1)
                  dup2(1, write_pipe[1]);
            }

				close(read_pipe[1]);
            close(read_pipe[0]);
            close(write_pipe[1]);
            close(write_pipe[0]);

            execvp(args[0], args);
            exit(EXIT_FAILURE);
         }
         if (ptr != NULL)
            ptr = ptr->next;
         else
            break;
      }
   } 
   
   free(args);
}

