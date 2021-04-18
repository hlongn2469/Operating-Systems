#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_LINE 80 /* The maximum length command */

void closeFile(int flag, int input_info, int output_info){
    if(flag & 1){
        close(input_info);
    }
    
    if(flag & 2){
        close(output_info);
    }

    
}

int redirect(int flag, char* file_input, char* file_output, int* input_info, int* output_info){
    // output redirection case
    if(flag & 2){
        *output_info = open(file_output, O_WRONLY | O_CREAT | O_TRUNC, 644);
        if(*output_info < 0){
            printf("Can't open output file");
            return 0;
        }
        dup2(*output_info, STDOUT_FILENO);
    }

    // input redirection case
    if(flag & 1){
        *input_info = open(file_input, O_RDONLY, 0644);
        if(*input_info < 0){
            printf("Can't open input file");
            return 0;
        }
        dup2(*input_info, STDIN_FILENO);
    }
    return 1;
}

int checkRedirect(char **arg, int* cmd_size, char **file_input, char**file_output){
    int flag = 0, remove_index = 0;
    int remove[4];
    for(int i = 0; i < *cmd_size; i++){
        if(remove_index >= 4){
            break;
        }

        // input redirect
        if(strcmp("<", arg[i]) == 0){
            remove[remove_index] = i;
            remove_index++;
            if(*cmd_size-1 == i){
                printf("No input file provided");
                break;
            }
            flag = flag | 1;
            *file_input = arg[i + 1];
            i++;
            remove[remove_index] = i;
            remove_index++;
        
        // output redirect
        } else if(strcmp(">", arg[i]) == 0){
            remove[remove_index] = i;
            remove_index++;
            if(*cmd_size-1 == i){
                printf("No output file provided");
                break;
            }

            flag = flag |= 2;
            *file_output = arg[i + 1];
            i++;
            remove[remove_index] = i;
            remove_index++;
        }
    }

    // remove I/O indicators and filesname from argument
    for(int i = remove_index - 1; i >= 0; i--){
        int index = remove[i];
        while(index != *cmd_size){
            arg[index] = arg[index+1];
            index++;
        }
        --*cmd_size;
    }
    return flag;
}

int checkAmp(int* cmd_size, char **arg){
    int length = strlen(arg[*cmd_size-1]);

    // & not detected
    if(arg[*cmd_size-1][length-1] != '&'){
        return 0;
    }

    // only contains &
    if(length == 1){
        free(arg[*cmd_size-1]);
        arg[*cmd_size-1] = NULL;
        --*cmd_size;

    } else {
        arg[*cmd_size -1][length-1] = '\0';
    }
    return 1;
}

int checkHistory(char *arg, char* cmd){
    if(strncmp(arg,"!!", 2) == 0){
        // no history case
        if(strlen(cmd) == 0){
            return 0;
        }
        // regular case, just print the command out
        printf("%s",cmd);
        return 1;
    }
    // update command
    strcpy(cmd,arg);
    return 1;
}

// get the last command from the input history
int checkValidInput(char *cmd){
    char arg[MAX_LINE+1];
    // no input case
    if(fgets(arg,MAX_LINE+1, stdin) == NULL){
        printf("INVALID INPUT");
        return 0;
    }

    if(!checkHistory(arg, cmd)){
        return 0;
    }
    
    return 1;
}

// parse input and store arguments
int parseInput(char* cmd, char *args[]){
    int arg_size = 0;
    char new_cmd[MAX_LINE+1];
    strcpy(new_cmd, cmd);

    // strip each token into sepearate argument and store it into arg array
    char *token = strtok(new_cmd, " \t\n\v\f\r");
    while(token != NULL){
        args[arg_size] = malloc(strlen(token)+1);
        strcpy(args[arg_size], token);
        arg_size++;
        token = strtok(NULL, " \t\n\v\f\r");
    }
    return arg_size;
}

int checkPipe(char** args, char***args2, int* cmd_size, int* cmd_size2){
    for(int i = 0; i < *cmd_size;i++){
        // pipe detected
        if(strcmp(args[i], "|") == 0){
            free(args[i]);
            args[i] = NULL;
            *cmd_size2 = *cmd_size - i - 1;
            *cmd_size = i;
            *args2 = args + i + 1;
            return 1;
        }
    }
    return 0;
}

int executePipe(char** args, char***args2, int* cmd_size, int* cmd_size2){

    // create pipe
    int fd[2];
    pipe(fd);

    // fork 2 other processes: child process for 2nd cmd AND grandchild process of 1st cmd
    pid_t process_id2 = fork();

    // child process for 2nd cmd
    if(process_id2 > 0){
        // redirect I/O
        char *file_input, *file_output;
        int input_info, output_info;
        int flag = checkRedirect(*args2, cmd_size2, &file_input, &file_output);

        // disable input redirection
        flag &= 2;

        if(redirect(flag, file_input, file_output, &input_info, &output_info) == 0){
            return 0;
        }

        // needs research
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        wait(NULL);
        execvp(*args2[0], *args2);
        closeFile(flag, input_info, output_info);
        close(fd[0]);
        fflush(stdin);

        // grandchild process for the 1st cmd
    } else if(process_id2 == 0){
        // redirect I/O
        char *file_input, *file_output;
        int input_info, output_info;
        int flag = checkRedirect(args, cmd_size, &file_input, &file_output);

        // disable output redirection
        flag = flag & 1; 

        if(redirect(flag, file_input, file_output, &input_info, &output_info) == 0){
            return 0;
        }

        // needs research
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        wait(NULL);
        execvp(args[0], args);
        closeFile(flag, input_info, output_info);
        close(fd[1]);
        fflush(stdin);
    }
}


int runCmd(char **args, int cmd_size){
    // detect & for concurrent running process first
    int concur = checkAmp(&cmd_size, args);

    // detect pipe
    int cmd_size2 = 0;
    char **args2;

    /**    
        * After reading user input, the steps are:    
        * (1) fork a child process using fork()    
        * (2) the child process will invoke execvp()    
        * (3) parent will invoke wait() unless command included &   
    */

    // create child process and execute the command
    pid_t process_id = fork();

    // fork unsucessfully
    if(process_id < 0){
        printf("fork failed");
        return 0;

    // child process 
    } else if (process_id == 0){
        // check pipe
        if(checkPipe(args, &args2, &cmd_size, &cmd_size2) == 1){
            executePipe(args, &args2, &cmd_size, &cmd_size2);
        // pipe undetected
        } else {
            // redirect I/O
            char *file_input, *file_output;
            int input_info, output_info;
            int flag = checkRedirect(args, &cmd_size, &file_input, &file_output);
            if(redirect(flag, file_input, file_output, &input_info, &output_info) == 0){
                return 0;
            }
            execvp(args[0], args);
            closeFile(flag, input_info, output_info);
            fflush(stdin);
        }
    
    // parent process
    } else {
        // wait if parent and child run concurrently
        if(!concur){
            wait(NULL);
        }
    }
    return 1;
}

// deallocate memory in args to avoid mem leaks
void emptyArgs(char *arg[]){
    for(int i = 0; i < MAX_LINE/2 + 1; i++){
        free(arg[i]);
        arg[i] = NULL;
    }
}

int main(void){  
    char command_line[MAX_LINE];
    char *args[MAX_LINE/2 + 1]; /* command line arguments */  
    int should_run = 1; /* flag to determine when to exit program */ 

    // initialize elements of args array
    for(int i = 0; i < MAX_LINE/2+1; i++){
        args[i] = NULL;
    }

    // initialize the command_line
    strcpy(command_line, "");

    while (should_run) {    
        printf("osh>");    
        fflush(stdout);    
        fflush(stdin);

        // empty args before parsing
        emptyArgs(args);

        // get user input
        if(checkValidInput(command_line)){
        
            // extract input for commands 
            int command_size = parseInput(command_line, args);

            // if user enter exit, the program is done
            if(strcmp("exit", args[0]) == 0){
                break;
            }

            // after successfully parse the user input, run the command
            runCmd(args, command_size);
        } else {
            continue;
        }
    }
    emptyArgs(args);  
    return 0;
}
