/*parseline written by Taylor Hayase and Ryan Premi*/
#ifndef PARSELINE_H
#define PARSELINE_H

#define CMDL_MAX 512
#define PIPE_MAX 10
#define CMD_MAX 10

typedef struct Stage
{
   int stage_num;
   char cmds[CMDL_MAX];
   char input[CMDL_MAX];
   char output[CMDL_MAX];
   int argc;
   char argv[CMD_MAX*2][CMDL_MAX];
   struct Stage *next;
}Stage;

Stage* parse_main(int, char*);

void create_cmds_list(char*, char [2*CMD_MAX][CMDL_MAX], int*);

Stage* set_stage(char[CMD_MAX][CMDL_MAX], int *, int, int);

void print_stage(Stage *);

void free_stages(Stage *);

int check_prog(char*);

void error_print(int, char*);

#endif
