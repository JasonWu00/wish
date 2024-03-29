#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "parse.h"

int main() {
  umask(0);
  int parentPID = getpid();

  printf("Opening shell.\n");
  printf("\n===========================\n");
  printf("\nWindows Shell v0.0.1, by Julian Wong and Jason Wu.\n");
  printf("\n===========================\n\n");
  printf("Notes:\n");
  printf("If you enter a nonexistent command, the shell will return an error message.\n");
  printf("Type \"exit\" to close the shell.\n\n");
  char input[100];
  char ** output;
  printf("WISH > ");

  while (1) {//as long as "exit" isn't entered
    fgets(input, 100, stdin);

    char *position;
    if ((position = strchr(input, '\n')) != NULL) {
      *position = '\0';
    }
    //gets input and replaces newline with null

    int numCommands;
    int pipe_num = -1;
    char *inptr = input;//pointer to input string
    char *commandArray[100];

    if (strchr(input, ';') != NULL) {//check if semicolon in input
      char *indcmd;
      int q = 0;
      while((indcmd = strsep(&inptr, ";")) != NULL) {
        commandArray[q] = indcmd;
        q++;
      }
      numCommands = q;
    }
    else if(strchr(input,'|') != NULL) {//check if pipe in input
      char *indcmd1;
      int q = 0;
      while((indcmd1 = strsep(&inptr,"|")) != NULL) {
        commandArray[q] = indcmd1;
        q++;
      }
      numCommands = q;
      pipe_num = 0;
    }
    else {
      commandArray[0] = inptr;
      numCommands = 1;
    }

    char cd[3] = "cd";
    char leave[5] = "exit";
    char lessthan = "<";
    char morethan = ">";
    char pipe = "|";

    int pipe_num = -1;

    const char *cd_p = &cd;
    const char *leave_p = &leave;
    const char *less_p = &lessthan;
    const char *more_p = &morethan;
    const char *pipe_p = &pipe;
    for(int q = 0; q < numCommands; q++) {//for every command entered

	    char * pointer = commandArray[q];
      struct parse_output outstruct;
      outstruct = parse_args(pointer);
      outstruct.lastToken--;

      //get locations of <, > in the output char** array
      int less_num = -1;
      int more_num = -1;
      for (int counter = 0; counter < outstruct.lastToken; counter++) {
        if (strcmp(outstruct.output[counter], "<") == 0) {
          //printf("Found a <\n");
          less_num = counter;
        }
        if (strcmp(outstruct.output[counter], ">") == 0) {
          //printf("Found a >\n");
          more_num = counter;
        }
      }
      //printf("DEBUG: less_num: %i, more_num: %i\n", less_num, more_num);

      //printf("DEBUG: your cmd: %s, last_token: %i\n", commandArray[q], outstruct.lastToken);
      //printf("Command has > sign: %i, 0 if DNE\n", strchr(outstruct.output, morethan) != NULL);
      //commented out printf that caused segfault
      if (strstr(commandArray[q], cd_p) != NULL) {//input command has a "cd" in it
        chdir(outstruct.output[1]);
      }

      else if (strstr(commandArray[q], leave_p) != NULL) {//input command is "exit"
        printf("\nWISH > Exiting shell\nThank you for visiting! Come again soon!\n\n");
        exit(0);
      }

      else if (strstr(commandArray[q], pipe_p) != NULL) {//check if pipe in input

        int pipe_num = -1;
        for (int counter = 0; counter < outstruct.lastToken; counter++) {
          if (strcmp(outstruct.output[counter], ";") == 0) {
            pipe_num = counter;
          }
        }

        char *commandBefore, *commandAfter;
        commandBefore = outstruct.output[pipe_num - 1];
        commandAfter = outstruct.output[pipe_num + 1];
        outstruct.output[pipe_num] = NULL;

        int fd_intermediatary = -1;
        fd_intermediatary = open("intermediatary", O_RDWR | O_CREAT | O_TRUNC, 0666);

        int new_fd_stdout = dup(1);
        int new_fd_stdin = dup(0);

        dup2(fd_intermediatary, 1);//replace stdout with intermediatary
        run_cmds(outstruct, parentPID);//runs first half, output goes into intermediatary
        dup2(fd_intermediatary, 0);//stdin with inter
        dup2(new_fd_stdout, 1);//reset stdout
        outstruct.output[pipe_num - 1] = outstruct.output[pipe_num + 1];
        run_cmds(outstruct, parentPID);
        dup2(new_fd_stdin, 0);//reset stdin
      }

      else if (less_num != -1 || more_num != -1) {//has a ">" or a "<", conduct redirection
        //printf("DEBUG: value of last token: %s\n", outstruct.output[outstruct.lastToken]);

        //strncat(outstruct.output[less_num], ".txt", 4);
        //strncat(outstruct.output[more_num], ".txt", 4);
        int fd_new_input = -1;
        int fd_new_output = -1;
        char *redirectionFileName;
        if (less_num != -1) {
          fd_new_input = open(outstruct.output[less_num - 1], O_RDWR | O_CREAT | O_TRUNC, 0666);
          redirectionFileName = outstruct.output[less_num - 1];
          //printf("DEBUG: less_num: %s\n", outstruct.output[less_num]);
          //printf("DEBUG: opening %s file\n", outstruct.output[less_num - 1]);
        }
        if (more_num != -1) {
          fd_new_output = open(outstruct.output[more_num + 1], O_RDWR | O_CREAT | O_TRUNC, 0666);
          redirectionFileName = outstruct.output[more_num + 1];
          //printf("DEBUG: more_num: %s\n", outstruct.output[more_num]);
          //printf("DEBUG: opening %s file\n", outstruct.output[more_num + 1]);
        }

        int new_fd_stdout = dup(1);
        int new_fd_stdin = dup(0);

        /*if (fd_new_input == -1 && less_num != -1) {//if no input file exists
          printf("Token at output: %s\n", outstruct.output[less_num - 1]);
          int fd_new_input = open(outstruct.output[less_num - 1], O_RDWR | O_CREAT, 0666);
          printf("DEBUG: fd_new_input: %i\n", fd_new_input);
        }
        if (fd_new_output == -1 && more_num != -1) {//no output file exists
          printf("Token at output: %s\n", outstruct.output[more_num + 1]);
          int fd_new_output = open(outstruct.output[more_num + 1], O_RDWR | O_CREAT, 0666);
          printf("DEBUG: fd_new_output: %i\n", fd_new_output);
        }*/
        if (less_num != -1) {
          dup2(fd_new_input, 0);//replaces stdin with new input file
          //printf("stdin replaced\n");
        }
        if (more_num != -1) {
          dup2(fd_new_output, 1);//now with stdout
          //printf("stdout replaced\n");
        }


        //The lines below are to prevent execvp from reading the < and/or >.

        outstruct.output[less_num] = NULL;
        outstruct.output[more_num] = NULL;

        //printf("entire array:\n");
        /*for (int counter = 0; counter <= outstruct.lastToken; counter++) {
          printf("%s", outstruct.output[counter]);
          printf("\n");
        }*/

        run_cmds(outstruct, parentPID);
        dup2(new_fd_stdin, 0);
        dup2(new_fd_stdout, 1);//undoes file table manipulation
        printf("WISH > Redirection to %s successful\n", redirectionFileName);
      }
      else if(pipe_num != -1) {//user inputs pipe |
        int fd_intermediatary = -1;
        fd_intermediatary = open("intermediatary", O_RDWR | O_CREAT | O_TRUNC, 0666);
        int new_fd_stdout = dup(1);
        int new_fd_stdin = dup(0);
      }
      else {
        run_cmds(outstruct, parentPID);
      }
    }
    printf("\nWISH > ");
  }
}
