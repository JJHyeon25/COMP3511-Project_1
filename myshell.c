/*
    COMP3511 Fall 2022 
    PA1: Simplified Linux Shell (MyShell)

    Your name: Ju Jong Hyeon
    Your ITSC email: jjuab@connect.ust.hk 

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks. 

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h> // For open/read/write/close syscalls

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters: 
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"
#define INPUT_CHAR "<"
#define OUTPUT_CHAR ">"

// Assume that we only have at most 8 pipe segements, 
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
//
// We also need to add an extra NULL item to be used in execvp
//
// Thus: 8 + 1 = 9
//
// Example: 
//   echo a1 a2 a3 a4 a5 a6 a7 
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT]; 
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO    0       // Standard input
#define STDOUT_FILENO   1       // Standard output 


 
// This function will be invoked by main()
// TODO: Implement the multi-level pipes below
void process_cmd(char *cmdline);

// read_tokens function is given
// This function helps you parse the command line
// Note: Before calling execvp, please remember to add NULL as the last item 
void read_tokens(char **argv, char *line, int *numTokens, char *token);

// Here is an example code that illustrates how to use the read_tokens function
// int main() {
//     char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
//     int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
//     char cmdline[MAX_CMDLINE_LEN]; // the input argument of the process_cmd function
//     int i, j;
//     char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL}; 
//     int num_arguments;
//     strcpy(cmdline, "ls | sort -r | sort | sort -r | sort | sort -r | sort | sort -r");
//     read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);
//     for (i=0; i< num_pipe_segments; i++) {
//         printf("%d : %s\n", i, pipe_segments[i] );    
//         read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);
//         for (j=0; j<num_arguments; j++) {
//             printf("\t%d : %s\n", j, arguments[j]);
//         }
//     }
//     return 0;
// }


/* The main function implementation */
int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    fgets(cmdline, MAX_CMDLINE_LEN, stdin);
    process_cmd(cmdline);
    return 0;
}

// TODO: implementation of process_cmd
void process_cmd(char* cmdline)
{
	// You can try to write: printf("%s\n", cmdline); to check the content of cmdline
	//printf("%s\n", cmdline);
	
	char* pipe_segments[MAX_PIPE_SEGMENTS];
	char* input_segments[MAX_CMDLINE_LEN];
	char* output_segments[MAX_CMDLINE_LEN];
	char* segments[MAX_CMDLINE_LEN];
	int num_pipe_segments, num_input_segments, num_output_segments, num_segments;
	char* arguments[MAX_ARGUMENTS_PER_SEGMENT] = { NULL };
	int num_arguments;
	char temp1[MAX_CMDLINE_LEN];
	strcpy(temp1,cmdline);
	char temp2[MAX_CMDLINE_LEN];
	strcpy(temp2,cmdline);
	read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);

	if (num_pipe_segments == 1) {
		//For non-pipes
		read_tokens(input_segments, cmdline, &num_input_segments, INPUT_CHAR);
		read_tokens(output_segments, temp1, &num_output_segments, OUTPUT_CHAR);
		if ((num_input_segments == 1) && (num_output_segments == 1)) {
			//For non <, >
			read_tokens(arguments, output_segments[0], &num_arguments, SPACE_CHARS);
			execvp(arguments[0], arguments);
		} else if ((num_input_segments == 1) && (num_output_segments != 1)) {
			//For single > (output)
			read_tokens(arguments, output_segments[1], &num_arguments, SPACE_CHARS);
			int fd = open(arguments[0], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
			close(1);
			dup2(fd, 1);
			read_tokens(arguments, output_segments[0], &num_arguments, SPACE_CHARS);
			execvp(arguments[0], arguments);
			close(fd);
		} else if ((num_input_segments != 1) && (num_output_segments == 1)) {
			//For single < (input)
			read_tokens(arguments, input_segments[1], &num_arguments, SPACE_CHARS);
			int fd = open(arguments[0], O_RDONLY, S_IRUSR | S_IWUSR);
			close(0);
			dup2(fd, 0);
			read_tokens(arguments, input_segments[0], &num_arguments, SPACE_CHARS);
			execvp(arguments[0], arguments);
			close(fd);
		} else {
			//For both < > and > <
			read_tokens(segments, temp2, &num_segments, SPACE_CHARS);
			int input = 0, output = 0;
			for (int i = 0; i < num_segments; ++i) {
				if (!strcmp(segments[i], OUTPUT_CHAR)) {
					output = i;
					break;
				}
			}
			for (int i = 0; i < num_segments; ++i) {
				if (!strcmp(segments[i], INPUT_CHAR)) {
					input = i;
						break;
				}
			}
			if (input < output) {
				//For < >
				read_tokens(arguments, segments[input + 1], &num_arguments, SPACE_CHARS);
				int fd1 = open(arguments[0], O_RDONLY, S_IRUSR | S_IWUSR);
				read_tokens(arguments, segments[output + 1], &num_arguments, SPACE_CHARS);
				int fd2 = open(arguments[0], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
				close(0);
				dup2(fd1, 0);
				close(1);
				dup2(fd2, 1);
				for (int i = input; i < num_segments; ++i) {
					segments[i] = NULL;
				}
				execvp(segments[0], segments);
				close(fd1);
				close(fd2);
			} else {
				//For > <
				read_tokens(arguments, segments[input + 1], &num_arguments, SPACE_CHARS);
				int fd1 = open(arguments[0], O_RDONLY, S_IRUSR | S_IWUSR);
				read_tokens(arguments, segments[output + 1], &num_arguments, SPACE_CHARS);
				int fd2 = open(arguments[0], O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
				close(0);
				dup2(fd1, 0);
				close(1);
				dup2(fd2, 1);
				for (int i = output; i < num_segments; ++i) {
					segments[i] = NULL;
				}
				execvp(segments[0], segments);
				close(fd1);
				close(fd2);
			}
		}
	}
	else {
		//Below is for single and multiple pipes
		int pfds[2];
		pid_t pid = 0;
		for (int i = 1; i < num_pipe_segments; ++i) {
			if (pid == 0) {
				pipe(pfds);
				pid = fork();
			}
			if (pid == 0) {
				read_tokens(arguments, pipe_segments[num_pipe_segments - i - 1], &num_arguments, SPACE_CHARS);
				arguments[num_arguments] = NULL;
				close(1);
				dup2(pfds[1], 1);
				close(pfds[0]);
				if ((num_pipe_segments - i - 1) == 0) {
					execvp(arguments[0], arguments);
				}
			}
			else {
				read_tokens(arguments, pipe_segments[num_pipe_segments - i], &num_arguments, SPACE_CHARS);
				arguments[num_arguments] = NULL;
				close(0);
				dup2(pfds[0], 0);
				close(pfds[1]);
				wait(0);
				execvp(arguments[0], arguments);
			}
		}
	}
}

// Implementation of read_tokens function
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}
