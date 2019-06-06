/*parseline written by Taylor Hayase and Ryan Premi*/

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "parseline.h"

/*run this to run parseline, return 0 on success*/
Stage* parse_main(int argc, char *argv)
{
   char c;
   int i = 0, num_cmds = 0, stages = 0, made = 0;
   char cmd_line[CMDL_MAX + 1];
   char args[2*CMD_MAX][CMDL_MAX];
   struct Stage *head = NULL, *ptr = NULL, *new;


   /*if reading commands from file*/
   if (argc > 1)
   {
      strcpy(cmd_line, argv);
      if (cmd_line[strlen(cmd_line) - 1] != '\0')
         cmd_line[strlen(cmd_line) - 1] = '\0';
   }
   else
   {
      /*read in from program*/
      while ((c = fgetc(stdin)) != '\n')
      {
         if ((c == ' ') && (i == 0))
            continue;

         if (c == '\n')
            break;
         
         if (c == EOF)
         {
            printf("^D\n");
            exit(0);
         }

         cmd_line[i] = c;
         i++;
      }
   

      if (cmd_line[i - 1] == ' ')
         cmd_line[i - 1] = '\0';
      else
         cmd_line[i] = '\0'; /*null terminate*/
   }

   if (strlen(cmd_line) == 0)
      return NULL;

   if (i >= CMDL_MAX) /*max of 512 bytes exceeded*/
   {
      error_print(1, "");
      return NULL;
   }

   create_cmds_list(cmd_line, args, &num_cmds);

   if (strlen(cmd_line) == 0)
      return NULL;

   i = 0;
   /*loop through all the commands*/
   while (i <= num_cmds)
   {
      /*check if a program*/
      made = 0;

      if (check_prog(args[i]))
      {
         /*if a program found make a stage*/
         i++;
         new = set_stage(args, &i, num_cmds, stages);
         stages++;
         made = 1;
      }

      if (stages == 0)
      {
         printf("%s: No such file or directory\n", args[0]);
         return NULL;
      }
      
      if (new == NULL)
         return NULL;
      /*add stage to linked list of stages*/
      if (made)
      {
         if (head == NULL)
         {
            head = new;
            ptr = head;
         }
         else
         {  
            ptr->next = new;
            ptr = ptr->next;
         } 
      }

      i++;

   }

   return head;
}

/*break command line from stdin to a list of arguments
 * removes spaces and error checks pipe depth and cmds*/
void create_cmds_list(char *cmd_line, char args[2*CMD_MAX][CMDL_MAX], int *num_cmds)
{
   int j = 0, i = 0;
   int pipes = 0;
   int max_cmds = 0;

   while(cmd_line[i] != '\0')
   {

      if (cmd_line[i] == ' ')
      {
         args[*num_cmds][j] = '\0';
         (*num_cmds)++;
         max_cmds++;
         j = 0;
         i++;
         continue;
      }
      if ( cmd_line[i] == '|')
      {
         pipes++;
         max_cmds = 0;
      }

      args[*num_cmds][j] = cmd_line[i];
      j++;
      i++;
   }
   args[*num_cmds][j] = '\0';


   if (pipes >= 10) /*pipeline too deep*/
   {
      error_print(2, "");
      cmd_line[0] = '\0';
   }

   if (max_cmds >= CMD_MAX - 1) /*commands exceed 10*/
   {
      error_print(3, "");
      cmd_line[0] = '\0';
   }
}

/*will return a new stage once a program has been found*/
Stage* set_stage(char args[CMD_MAX][CMDL_MAX], int *i, int num_cmds, int stg)
{
   struct Stage *stage = calloc(1, sizeof(Stage));
   struct stat sb;
   char buff[CMDL_MAX];
   int x = 0;

   /*initialize stage*/
   stage->next = NULL;

   strcpy(stage->input, "stdin");
   strcpy(stage->output, "stdout");
   strcpy(stage->argv[x], "\0");
   strcpy(stage->cmds, "\" ");
   stage->stage_num = stg;
  
   /*if command is a single program*/
   if (num_cmds == 0)
   {
      stage->argc++;

      strcat(stage->cmds, args[(*i) - 1]);
      strcat(stage->cmds, "\"");

      strcat(stage->argv[x], args[(*i) - 1]);
      x++;

      return stage;
   }

   /*if reached the end of commands, and last command is a program*/
   if (*i == num_cmds + 1)
   {
      (*i)--;

      stage->argc++;

      strcat(stage->cmds, args[*i]);
      strcat(stage->cmds, " \"");

      strcat(stage->argv[x], args[*i]);
      x++;

      /*if there was a pipe to last program*/
      if (args[(*i) - 1][0] == '|')
      {
         /*printf("input from pipe\n");*/
         strcpy(stage->input, "pipe from stage ");

         sprintf(buff, "%d", (stage->stage_num) - 1);
         strcat(stage->input, buff);
      }
      (*i)+=2;
      stage->argv[x][0] = '\0';
      return stage;
   }

   /*until reach next program, look for redirects, pipes and regular args*/
   while (!check_prog(args[*i]) && ((*i) <= num_cmds))
   {
      /*if last args was the program, add to stage cmds and argv*/
      if (check_prog(args[(*i) - 1]))
      {
         /*printf("inside you\n");*/

         strcat(stage->argv[x], args[(*i) - 1]);
         x++;

         stage->argc++;

         strcat(stage->cmds, args[(*i) - 1]);
         strcat(stage->cmds, " ");
      }
      /* check if input was piped from elsewhere*/
      if (args[(*i) - 2][0] == '|')
      {

         if (args[*i][0] == '<')
         {
            error_print(7, args[(*i) - 1]);
            return NULL;
         }

         /*printf("input from pipe\n");*/
         strcpy(stage->input, "pipe from stage ");

         sprintf(buff, "%d", (stage->stage_num) - 1);
         strcat(stage->input, buff);

      }

      /*if input redirect, check file exists, and update input*/
      if (args[*i][0] == '<')
      {

      /*error handling*/
         if (args[(*i) + 2][0] == '<')
         {
            error_print(5, args[(*i) + 1]);
            return NULL;
         }

         /*update stage cmds*/
         strcat(stage->cmds, "< ");
         (*i)++;

        /* printf("i is now = %d\n", *i);*/

         if (*i > num_cmds)
            break;

         /*update command line*/
         strcat(stage->cmds, args[*i]);
         strcat(stage->cmds, " ");

         /*check if file exitst*/
         if (lstat(args[*i], &sb) == -1)
         {
            perror("stat");
            exit(EXIT_FAILURE);
         /*print redirect error*/
         }
         /*change stdin for stage*/
         strcpy(stage->input, args[*i]);
      }  

      /*if output redirect, check if file exists and such*/
      else if (args[*i][0] == '>')
      {
         /*error handling*/
         if (args[(*i) + 2][0] == '>')
         {
            error_print(5, args[(*i)+1]);
            return NULL;
         }

         if (args[(*i) + 2][0] == '|')
         {
            error_print(7, args[(*i)+1]);
            return NULL;
         }

         /*update command line*/
         strcat(stage->cmds, "> ");
         (*i)++;

         if (*i > num_cmds)
            break;

         strcat(stage->cmds, args[*i]);
         strcat(stage->cmds, " ");

         /*either file exists so truncate, or create if not*/
         if (open(args[*i], O_TRUNC | O_CREAT, 0666) == -1)
         {
            printf("file create failed\n");
            /*print redirect error*/
         }
         /*change stdout*/
         strcpy(stage->output, args[*i]);
      }

      /*check for piping*/
      else if (args[*i][0] == '|')
      {

         /*error handling*/
         if ((args[(*i) + 1][0] == '|'))
         {
            error_print(4, "");
            return NULL;
         }

         /*update pipe output*/
         strcpy(stage->output, "pipe to stage ");

         sprintf(buff, "%d", (stage->stage_num) + 1);
         strcat(stage->output, buff);

         strcat(stage->cmds, "\"");
         stage->argv[x][0] = '\0';

         /*if a pipe, no other arguments, stage finished*/
         return stage; 
      }

      else
      {
         /*otherwise, part of argv for this command*/
         stage->argc++;

         strcat(stage->argv[x], args[*i]);
         x++;

         strcat(stage->cmds, args[*i]);
         strcat(stage->cmds, " ");
      }
      (*i)++;

      if (*i > num_cmds)
         break;
   }
   
   strcat(stage->cmds, "\"");
   stage->argv[x][0] = '\0';
   return stage;

}

/*formatted output*/
void print_stage(Stage *head)
{
   Stage *ptr;
   int x = 0;

   ptr = head;
   while (ptr != NULL)
   {
      printf("\n--------\n");
      printf("Stage %d: %s\n", ptr->stage_num, ptr->cmds);
      printf("--------\n");
      printf("input: %s\n", ptr->input);
      printf("output: %s\n", ptr->output);
      printf("argc: %d\n", ptr->argc);
      printf("argv: ");

      while (ptr->argv[x][0] != '\0')
      {
         printf("\"%s\"", ptr->argv[x]);
         x++;
         if (ptr->argv[x][0] != '\0')
            printf(",");
      }

      printf("\n");

      ptr = ptr->next;
      x = 0;
   }
}

/*free linked list when done*/
void free_stages(Stage *head)
{
   Stage *ptr;

   while (head != NULL)
   {
      ptr = head;
      head = head->next;

      free(ptr);
   }
}

/*check if valid program, return 1 if true*/
int check_prog(char *args)
{
   int pid = 0, fd = 0;
   int childStatus = 0;

   fd = creat("debuggger", 0666);

   if (strcmp(args, "tee") == 0 || strcmp(args, "cat") == 0 || strcmp(args, "sort") == 0)
   {
      remove("debuggger");
      return 1;
   }

   if ((pid = fork()) == 0) 
   { 
/*child process, change output from stdout*/
      dup2(fd, 1);
      dup2(fd, 2);
      close(fd);


     /* execlp(str, str, (char*)0);*/
      execlp(args, args, NULL);
         /*exit if error*/
      exit(-1);
   } 
   else 
   {       
      waitpid(pid, &childStatus, 0);
      close(fd);

      if(WEXITSTATUS(childStatus) > 5)
      {
         remove("debuggger");
         return 0;
      }
         /*process does exist*/
   }

   remove("debuggger");
   return 1;
}

/*error handler*/
void error_print(int err, char *str)
{
   if (err == 1)
      fprintf(stderr, "command too long\n");
   else if (err == 2)
      fprintf(stderr, "pipeline too deep\n");
   else if (err == 3)
      fprintf(stderr, "too many arguments\n");
   else if (err == 4)
      fprintf(stderr, "invalid null command\n");
   else if (err == 5)
      fprintf(stderr, "%s: bad input redirection\n", str);
   else if (err == 6)
      fprintf(stderr, "%s: bad output redirection\n", str);
   else if (err == 7)
      fprintf(stderr, "%s: ambiguous input\n", str);
   else if (err == 8)
      fprintf(stderr, "%s: ambiguous output\n", str);
}
      
