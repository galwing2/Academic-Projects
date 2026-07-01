#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>

void mySignalHandler(int signal){
    int status;
    while(waitpid(-1,&status,WNOHANG)>0){ /*Waits for all child processes, WNOHANG signal means waitpid is non blocking (from selg learning documentation)*/
    }
}

int prepare(void){
    signal(SIGINT,SIG_IGN); /*Make parent shell ignore ctrl+C so it doesn't terminate opon SIGINT*/

    struct sigaction newAction = {.sa_handler=mySignalHandler}; /*Create signal handler struct like in the self learning pdf*/
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = SA_RESTART; /*Automatically retry interrupted blocking system calls after the signal handler returns*/
    if(sigaction(SIGCHLD,&newAction,NULL)==-1){ /*Overwrite default behavior for ctrl+C*/
        perror("Signal handle registration failed");
        exit(1);
    }
    return 0;
}


int finalize(void) {
    return 0;
}

int run_regular_command(char **arglist){
    pid_t pid = fork();
    if (pid < 0){/*Fork failed*/
        perror("Fork failed");        
        return 0;
    } 
    else{
        if (pid == 0){ /*Child block*/
            signal(SIGINT,SIG_DFL); /*Make foreground child processes terminate upon SIGINT*/
            if(execvp(arglist[0], arglist)==-1){ /*Execvp failed or command was not found (execvp never returns if successful)*/
                perror("Command not found");
                exit(1);
            }
        }
        else{ /*Parent block*/
                int status;
                waitpid(pid,&status,0); /*Write status and 0 option to make shell wait for specific child process to finish*/
        }
    }
    return 1;

}
int run_background_command(char **arglist){
    pid_t pid = fork();
    if (pid < 0){/*Fork failed*/
        perror("Fork failed");
        return 0;
    }     
    else{
        if (pid == 0){ /*Child block*/
            if(execvp(arglist[0], arglist)==-1){ /*Execvp failed or command was not found (execvp never returns if successful)*/
                perror("Command not found");
                exit(1);
            }
        }
    }
    return 1;
}
int run_input_output_command(int count,char **arglist,bool file_type){
    int idx = -1;
    char *file = NULL;
    char *command = "<";
    int fd = -1;
    if(!file_type){command = ">";} /*Assertain if command is inout or output redirect*/
    for(int i=0;i<count;i++){
        if(strcmp(arglist[i],command)==0){
            idx = i;
            break;
        }
    }
    if(idx==-1){ /*Meaning no input/output special symbol was found, user error*/
        fprintf(stderr, "Illegal input\n");        
        return 1;
    }
    file = arglist[idx+1];
    arglist[idx] = NULL;

    pid_t pid = fork();
    if (pid < 0){/*Fork failed*/
        perror("Fork failed");
        return 0;
    }     
    else{
        if (pid == 0){ /*Child block*/
            signal(SIGINT,SIG_DFL);/*Child inherits parents signal control so we need to reset in order to terminate upon SIGINT*/
            if(file_type){fd = open(file, O_RDONLY, S_IRUSR | S_IWUSR);}
            else{fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);}
            if (fd < 0) {
                if (file_type) perror("Failed to open input file");
                else perror("Failed to open output file");
                exit(1);
            }
            if(file_type){dup2(fd,0);}
            else{dup2(fd,1);}
            close(fd);
            if(execvp(arglist[0], arglist)==-1){ /*Execvp failed or command was not found (execvp never returns if successful)*/
                perror("Command not found");
                exit(1);
            }
        }
        else{ /*Parent block*/
            int status;
            waitpid(pid,&status,0); /*Write status and 0 option to make shell wait for specific child process to finish*/
        }
    }
    return 1;
}

int run_pipe_command( int count, char **arglist){
    int pipe_count = 1;
    int command_idx=0; /*Keep track of which command we are at for arglist to be passed correctly to execvp*/
    int prev_pipe = 0; /*To track where we are in command order*/
    for(int i=0;i<count;i++){ /*count how many pipe commands in order to keep track of shell piping*/
        if(strcmp(arglist[i],"|")==0){
            pipe_count++;
            arglist[i] = NULL; /*Create "mini" arglists so we can just pass them to execvp in between NULL terminators*/
        }
    }
    if(pipe_count>10){
        fprintf(stderr, "too many pipe symbols\n");
        return 1;
    }
    pid_t pids[pipe_count];
    for(int i=0;i<pipe_count;i++){
        int pfds[2];
        if(i<pipe_count-1){ /*If we are not at last command, create a new pipe*/
            if(pipe(pfds)==-1){
                perror("Pipe failed");
                if(prev_pipe != 0) close(prev_pipe); /* Cleanup before return, close read end */
                for(int j=0; j<i; j++){ /* Wait for all previously created children */
                    waitpid(pids[j], NULL, 0); 
                }
                return 0;
            }
        }
        pid_t pid = fork();
        if (pid < 0){/*Fork failed*/
            perror("Fork failed");
            
            if(prev_pipe != 0) close(prev_pipe);  /* Cleanup before return, close read end */
            if(i<pipe_count-1){ /* Close the pipe we just created but didn't use */
                close(pfds[0]);
                close(pfds[1]);
            }
            for(int j=0; j<i; j++){ /* Wait for all previously created children */
                waitpid(pids[j], NULL, 0); 
            }
            return 0;
        }
        else{
            if(pid==0){ /*Child process, need to connect to new pipe*/
                signal(SIGINT,SIG_DFL); /*Child inherits parents signal control so we need to reset in order to terminate upon SIGINT*/
                if(prev_pipe!=0){ /*If this isn't the first command, prev_pipe is the read end of previous pipe and we need to read from previous pipe*/
                    dup2(prev_pipe, 0);
                    close(prev_pipe);
                }
                if(i<pipe_count-1){/*If this is not the last command we need to write to the new pipe*/
                    dup2(pfds[1], 1);
                    close(pfds[0]);
                    close(pfds[1]);
                }
                if(execvp(arglist[command_idx],&arglist[command_idx])==-1){ /*Execute current command, keeping track of correct arglist parts*/
                    perror("Command not found");
                    exit(1);
                }
            }
            else{ /*Parent process*/
                if(prev_pipe!=0){ /*If this isn't the first command we can close the old read end of the pipe so pipe doesn't stay open unnecessarily*/
                    close(prev_pipe);
                }
                if(i<pipe_count-1){ /*If this isn't the last command, we need to save the new read end for the next pipe*/
                    prev_pipe = pfds[0];
                    close(pfds[1]); /*Must close old write end*/
                }
                while(arglist[command_idx]!=NULL){ /*Iterate to next command start*/
                    command_idx++;
                }
                command_idx++; /*One index after NULL is the next command start*/
            }
        }
    }
    int status;
    for(int i=0;i<pipe_count;i++){  /*Wait for all pipe inititated child processes to end, so we only wait for them and not background commands as well*/
        waitpid(pids[i],&status,0);
    }
    return 1;
}


int process_arglist(int count, char **arglist){
    bool check_pipe = false;
    bool check_regular = false;
    bool check_background = false;
    bool check_input = false;
    bool check_output = false;
    for(int i=0;i<count;i++){ /*Determines type of command*/
        if (strcmp(arglist[i],"|")==0) check_pipe = true;
        if (strcmp(arglist[i],"&")==0) check_background = true;
        if (strcmp(arglist[i],"<")==0) check_input = true;
        if (strcmp(arglist[i],">")==0) check_output = true;
    }
    if (!check_pipe && !check_background && !check_input && !check_output) check_regular = true;
    if (check_regular==true){/*Handles case of executing regular command, i.e. not a special operation*/
        return run_regular_command(arglist); 
    }

    if (check_pipe==true){ /*Handles case of pipe command*/
        return run_pipe_command(count,arglist);
    }
    if (check_background==true){/*Handles case of executing commands in background*/
        arglist[count-1]=NULL; /*To make sure we dont pass & to execvp*/
        return run_background_command(arglist);

    }
    if (check_input==true){ /*Handles case of input redirecting command*/
        return run_input_output_command(count,arglist,true);
    }
    if (check_output==true){ /*Handles case of output redirecting command*/
        return run_input_output_command(count,arglist,false);
    }
    return 1;
}