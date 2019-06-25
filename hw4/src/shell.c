#include "../include/shell_util.h"
#include "../include/linkedList.h"
#include "../include/helpers.h"

// Library Includes
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

int terminated_pid = 0;

void sigchild_handler();
void printBG();
List_t bg_list;

int main(int argc, char *argv[])
{
	int i; //loop counter
	char *args[MAX_TOKENS + 1];
	int exec_result;
	int exit_status;
	pid_t pid;
	pid_t wait_result;
    
    //Initialize the linked list
    bg_list.head = NULL;
    bg_list.length = 0;
    bg_list.comparator = comparator;  // Don't forget to initialize this to your comparator!!!

	// Setup segmentation fault handler
	if(signal(SIGSEGV, sigsegv_handler) == SIG_ERR)
	{
		perror("Failed to set signal handler");
		exit(-1);
	}

	if(signal(SIGCHLD, sigchild_handler) == SIG_ERR)
	{
		perror("Failed to set signal handler");
		exit(-1);
	}

	if(signal(SIGUSR1, printBG) == SIG_ERR)
	{
		perror("Failed to set signal handler");
		exit(-1);
	}



	while(1) {


		done:;
		// DO NOT MODIFY buffer
		// The buffer is dynamically allocated, we need to free it at the end of the loop
		char * const buffer = NULL;
		size_t buf_size = 0;

		// Print the shell prompt
		display_shell_prompt();

		// Read line from STDIN
		ssize_t nbytes = getline((char **)&buffer, &buf_size, stdin);
		
		
		if(terminated_pid != 0)
		{

			node_t *pNode = bg_list.head;
			int i;
			for(i =0; i < bg_list.length; i++)
			{
			    pid_t tmpPid = ((ProcessEntry_t*)(pNode->value))->pid;
                int i1 = kill(tmpPid, 0 );
                if(i1 == -1)
				{
					fprintf(stdout, BG_TERM, ((ProcessEntry_t*)(pNode->value))->pid, ((ProcessEntry_t*)(pNode->value))->cmd);
					removeByPid(&bg_list, ((ProcessEntry_t*)(pNode->value))->pid);
					//free(((ProcessEntry_t*)(pNode->value))->cmd);
				}
                pNode = pNode->next;
            }

			terminated_pid = 0;
		}
		

		// No more input from STDIN, free buffer and terminate
		if(nbytes == -1) {
			free(buffer);
			break;
		}

		// Remove newline character from buffer, if it's there
		if(buffer[nbytes - 1] == '\n')
			buffer[nbytes- 1] = '\0';

		// Handling empty strings
		if(strcmp(buffer, "") == 0) {
			free(buffer);
			continue;
		}
		char * str = (char*)malloc(sizeof(char)*80);
		strcpy(str, buffer);
		// Parsing input string into a sequence of tokens
		size_t numTokens;
		*args = NULL;
		numTokens = tokenizer(buffer, args);

		if(strcmp(args[0],"exit") == 0) {
			node_t *pNode = bg_list.head;
			for(i = 0; i < bg_list.length; i++)
			{
				pid_t pid1 = ((ProcessEntry_t*)(pNode->value))->pid;
				kill(pid1, SIGQUIT);
                fprintf(stdout, BG_TERM, ((ProcessEntry_t*)(pNode->value))->pid, ((ProcessEntry_t*)(pNode->value))->cmd);
				pNode = pNode->next;
			}
			// Terminating the shell
			free(buffer);
			return 0;
		}


		if(strcmp(args[0], "cd") == 0){
		    char *pathvar =NULL;
            if(args[1] == NULL)
            {
                pathvar = getenv("HOME");
            } else
            {
                pathvar = args[1];
            }


            if(chdir(pathvar) == -1)
            {
                fprintf(stderr, DIR_ERR);
            } else
            {
                char buf[80];
                getcwd(buf, sizeof(buf));
                printf("%s\n", buf);
            }
            free(buffer);
            continue;

		}

		if(strcmp(args[0], "estatus") == 0)
        {
		    printf("%d\n", (short)exit_status);
            free(buffer);
            continue;
        }


		int j;
		for(j = 0; j < numTokens; j++)
		{
			if(strcmp(args[j], "|") == 0)
			{

			    if(j == numTokens-1)
                {
			        fprintf(stderr, PIPE_ERR);
			        goto done;
                }
				args[j] = NULL;
				int fd[2];
				pipe(fd);

                for (i = 0; i < 2; i++)
                {
                    pid = fork();
                    if(pid == 0)
                        break;

                }

                if(i == 0)
                {
                    close(fd[0]);
                    dup2(fd[1], STDOUT_FILENO);
                    exec_result = execvp(args[0], &args[0]);
                    if(exec_result == -1){ //Error checking
                        printf(EXEC_ERR, args[0]);
                        exit(EXIT_FAILURE);
                    }
                    exit(EXIT_SUCCESS);
                }else if(i == 1)
                {
                    if(strcmp(args[numTokens-1], "&") == 0)
                    {
                        args[numTokens-1] = NULL;
                    }
                    close(fd[1]);
                    dup2(fd[0], STDIN_FILENO);
                    exec_result = execvp(args[j+1], &args[j+1]);
                    if(exec_result == -1){ //Error checking
                        printf(EXEC_ERR, args[0]);
                        exit(EXIT_FAILURE);
                    }
                    exit(EXIT_SUCCESS);
                }else if(i == 2)
                {
                    close(fd[0]);
                    close(fd[1]);

                    wait_result = waitpid(pid, &exit_status, 0);
                    if(wait_result == -1){
                        printf(WAIT_ERR);
                        exit(EXIT_FAILURE);
                    }

                }
				free(buffer);
				goto done;

			}
		}










        if(numTokens > 1 && strcmp(args[numTokens-1], "&") == 0)
        {


			pid = fork();   //In need of error handling......

			if (pid == 0){ //If zero, then it's the child process

                int i;
                int in = 0;
                int out = 0;
                int err = 0;
                char* file1 = NULL;
                char* file2 = NULL;
                char* file3 = NULL;
                for(i = 0; i < numTokens; i++)
                {

                    if(strcmp(args[i], "<") == 0)
                    {
                        if(in != 0)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }

                        if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }


                        in = open(args[i + 1], O_RDONLY);
                        if(in < 0)
                        {
                            fprintf(stderr, "REDIRECTION ERROR: Invalid operators or file combination.\n");
                            goto done;
                        }
                        dup2(in, STDIN_FILENO);
                        file1 = args[i+1];
                        args[i] = NULL;
                        i++;
                    }

                    if(strcmp(args[i], ">") == 0)
                    {
                        if(out != 0)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }

                        if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }
                        out = open(args[i + 1], O_WRONLY |O_CREAT, 0755);
                        dup2(out, STDOUT_FILENO);
                        file2 = args[i+1];
                        args[i] = NULL;
                        i++;
                    }

                    if(strcmp(args[i], "2>") == 0)
                    {
                        if(err != 0)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }

                        if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }
                        err = open(args[i + 1], O_WRONLY| O_CREAT);
                        dup2(err, STDERR_FILENO);
                        file3 = args[i+1];
                        args[i] = NULL;
                        i++;
                    }

                    if(strcmp(args[i], ">>") == 0)
                    {
                        if(out != 0)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }

                        if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                        {
                            fprintf(stderr, RD_ERR);
                            goto done;
                        }
                        err= open(args[i + 1], O_WRONLY| O_CREAT | O_APPEND);
                        dup2(err, STDOUT_FILENO);
                        file2 = args[i+1];
                        args[i] = NULL;
                        i++;
                    }


                }

				args[numTokens-1] = NULL;
				exec_result = execvp(args[0], &args[0]);
				//kill(getppid(), SIGCHLD);
				if(exec_result == -1){ //Error checking
					printf(EXEC_ERR, args[0]);
					exit(EXIT_FAILURE);
				}

				exit(EXIT_SUCCESS);
			}



			ProcessEntry_t* entry = (ProcessEntry_t*)malloc(sizeof(ProcessEntry_t));
			entry->cmd = str;
			time(&entry->seconds);
			entry->pid = pid;
			insertRear(&bg_list, entry);
            int status;
            waitpid(pid, &status, WNOHANG);
			continue;
        }



		pid = fork();   //In need of error handling......

		if (pid == 0){ //If zero, then it's the child process
            int i;
            int in = 0;
            int out = 0;
            int err = 0;
            char* file1 = NULL;
            char* file2 = NULL;
            char* file3 = NULL;
            for(i = 0; i < numTokens; i++)
            {

                if(strcmp(args[i], "<") == 0)
                {
                    if(in != 0)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }

                    if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }


                    in = open(args[i + 1], O_RDONLY);
                    if(in < 0)
                    {
                        fprintf(stderr, "REDIRECTION ERROR: Invalid operators or file combination.\n");
                        goto done;
                    }
                    dup2(in, STDIN_FILENO);
                    file1 = args[i+1];
                    args[i] = NULL;
                    i++;
                }

                if(strcmp(args[i], ">") == 0)
                {
                    if(out != 0)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }

                    if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }
                    out = open(args[i + 1], O_WRONLY |O_CREAT, 0755);
                    dup2(out, STDOUT_FILENO);
                    file2 = args[i+1];
                    args[i] = NULL;
                    i++;
                }

                if(strcmp(args[i], "2>") == 0)
                {
                    if(err != 0)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }

                    if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }
                    err = open(args[i + 1], O_WRONLY| O_CREAT);
                    dup2(err, STDERR_FILENO);
                    file3 = args[i+1];
                    args[i] = NULL;
                    i++;
                }

                if(strcmp(args[i], ">>") == 0)
                {
                    if(out != 0)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }

                    if(args[i+1] == file1 || args[i+1] == file2 || args[i+1] == file3)
                    {
                        fprintf(stderr, RD_ERR);
                        goto done;
                    }

                    err= open(args[i + 1], O_WRONLY| O_APPEND);
                    if(err < 0)
                    {
                        err = open(args[i + 1], O_WRONLY| O_CREAT);
                    }
                    dup2(err, STDOUT_FILENO);
                    file2 = args[i+1];
                    args[i] = NULL;
                    i++;
                }


            }

            exec_result = execvp(args[0], &args[0]);
			if(exec_result == -1){ //Error checking
				printf(EXEC_ERR, args[0]);
				exit(EXIT_FAILURE);
			}
		    exit(EXIT_SUCCESS);
		}
		 else{ // Parent Process
			wait_result = waitpid(pid, &exit_status, 0);
			if(exit_status == 256)
			    exit_status = 1;
			if(wait_result == -1){
				printf(WAIT_ERR);
				exit(EXIT_FAILURE);
			}
		}



		// Free the buffer allocated from getline
		free(buffer);
	}
	return 0;
}


void sigchild_handler()
{
    terminated_pid = 1;
}


void printBG()
{
	node_t *pNode = bg_list.head;
	int i;
	for(i = 0; i < bg_list.length; i++)
	{
	    printBGPEntry((ProcessEntry_t*)pNode->value);
		pNode = pNode->next;
	}
}
