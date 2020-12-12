#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include "parsecline.h"
#include "orphanage.h"

int status_term = -1; //contains status of terminated
int status_exit = -1; //contains status of exited
int fg_only = 0;      //contains whether in foreground only mode (0 = fg only, 1 = fg and bg)

//print the parsed command line arguments (used for error handling)
void print_parsed(struct cla* ul){
    //print contents
    printf("%d<-id, %d<-bg, %s<-command, %s<-input, %s<-output, ", ul->id, ul->bg, ul->command, ul->input, ul->output);
    for (int i = 0; i < 512; ++i){
        if (strcmp(ul->arguments[i], "\0") != 0)
            printf("%s<-%luargu\t", ul->arguments[i], strlen(ul->arguments[i]));
    }
    printf("\n");
}

//parses through command line to get input
//stores input into in
//updates curr and prev pointers
void take_input(char* cline, char* in, int* curr, int* prev){
    //increment curr, until you reach a non space or at the end of input
    while((*curr < strlen(cline)) && (cline[(*curr)] == ' ' && cline[(*curr)] != '\n'))
        (*curr)++;
    
    //if curr is at the end and the character is a white space, null, or newline, just clear input
    if ((*curr == strlen(cline)-1) && (cline[(*curr)] == ' ' || cline[(*curr)] == '\n' || cline[(*curr)] == '\0')){
        memset(in, '\0', 256);
    }else{
        *prev = *curr;
        //go until we dont find a valid character
        while((*curr < strlen(cline)-1) && (cline[(*curr)] != ' ' && cline[(*curr)] != '\n' && cline[(*curr)] != '\0')){
            (*curr)++;
        }

        //copy input into in
        char* hold = malloc(256);
        memset(hold, '\0', 256);
        strncpy(hold, cline+(*prev), (*curr)-(*prev));
        strncpy(in, hold, strlen(hold));
        free(hold);
    }

}

//parses through command line to get output
//stores output into out
//updates curr and prev pointers
void take_output(char* cline, char* out, int* curr, int* prev){
    //increment curr, until you reach a non space or at the end of input
    while((*curr < strlen(cline)) && (cline[(*curr)] == ' ' && cline[(*curr)] != '\n'))
        (*curr)++;
    
    //if curr is at the end and the character is a white space, null, or newline, just clear input
    if ((*curr == strlen(cline)-1) && (cline[(*curr)] == ' ' || cline[(*curr)] == '\n' || cline[(*curr)] == '\0')){
        memset(out, '\0', 256);
    }else{
        *prev = *curr;
        //go until we dont find a valid character
        while((*curr < strlen(cline)-1) && (cline[(*curr)] != ' ' && cline[(*curr)] != '\n' && cline[(*curr)] != '\0')){
            (*curr)++;
        }

        //copy output into out
        char* hold = malloc(256);
        memset(hold, '\0', 256);
        strncpy(hold, cline+(*prev), (*curr)-(*prev));
        strncpy(out, hold, strlen(hold));
        free(hold);
    }
}

//takes user input given to the shell and parses it 
//Note* assuming that command given is in correct syntax
void parse_commandline(char* cline, char* command, char** arguments, char* in, char* out, int* bg){
    int clen = strlen(cline);
    //check if valid background command 
    if ((clen > 3) && (cline[clen-2] == '&' && cline[clen-3] == ' '))
        *bg = 1;


    //reach the end of the command and store in command string
    int curr = 0, prev = 0;
    while((curr < clen) && (cline[curr] != ' ' && cline[curr] != '\n')) 
        ++curr;
    strncpy(command, cline, curr);
    prev = curr;


    curr++;
    int args = 0;
    //search for arguments in command line and store in arguments array
    while((curr < clen) && (cline[curr] != '<' && cline[curr] != '>')){
        //reach end of argument, check if its empty, then store in arguments
        if (cline[curr] == ' ' || cline[curr] == '\n'){
            char* hold = malloc(256);
            memset(hold, '\0', 256);
            strncpy(hold, cline+prev+1, curr-prev-1);
            
            //ignore any parsed strings into hold if it is blank
            int empty = 0;
            for (int i = 0; i < strlen(hold); ++i){
                if (hold[i] == ' ')
                    empty++;
            }
            
            //storing arguments, ignores if hold is a " "
            if(empty != strlen(hold) && args < 512 ){
            // ignores if argument comes up as &
                if (strncmp(hold, "&", strlen(hold)) != 0){
                    strncpy(arguments[args], hold, 256);
                    args++;
                }
            }
                  
            free(hold);
            prev = curr;
        }
        curr++;
    }

    //check whether input or output and then parse it
    if (cline[curr] == '>' || cline[curr] == '<'){
        prev = curr;
        if (cline[curr] == '<'){
            curr++;
            take_input(cline, in, &curr, &prev);
        }else if (cline[curr] == '>'){
            curr++;
            take_output(cline, out, &curr, &prev);
        }
    }

    //increment curr until it reaches < or >
    while((curr < clen) && (cline[curr] != '<' && cline[curr] != '>'))
        curr++;

    //check if we will parse for input or output and then store respectively
    if (cline[curr] == '>' || cline[curr] == '<'){
        prev = curr;
        if (cline[curr] == '<'){
            curr++;
            take_input(cline, in, &curr, &prev);
        }else if (cline[curr] == '>'){
            curr++;
            take_output(cline, out, &curr, &prev);
        }
    }

}

//checks if command line input is full of white spaces, new line, or null , #
//if it has as many characters as the clen then the command line is considered to be empty and will not be parsed
//Note* Will interpret command line has entered a comment if syntax is correct(i.e. # is first char)
//returns 0 if command line is not empty, 1 if command line is empty
int no_command(char* cline){
    int clen = strlen(cline);
    int empty = 0, count = 0;
    
    //evaluate the input as empty if the first char is a comment (#)
    if (cline[0] != '#'){
        for (int i = 0; i < clen; ++i){
            if (cline[i] == '\0' || cline[i] == '\n' || cline[i] == '#' || cline[i] == ' ')
                count++;
        }
        if (count == clen)
            empty = 1;
    }else
        empty = 1;

    return empty;
}

//given a string if it contains an instance of $$ it will update the string to carry the expanded variable
void expand_variable(char* string, int pid){
    //string has to be at least contain two characters
    if (strlen(string) > 1){
        char* hold = malloc(256);
        memset(hold, '\0', 256);

        //convert pid to string
        char* num = malloc(256);
        memset(num, '\0', 256);
        sprintf(num, "%d", pid);

        //look through every char in string
        for (int i = 0; i < strlen(string); ++i){
            if (string[i] == '$'){
                if (i+1 < strlen(string) && string[i+1] == '$'){
                    //add all contents of num to end of hold
                    strncpy(hold+strlen(hold), num, strlen(num));
                    i++;
                }else{
                    //copy current character to hold
                    strncpy(hold+strlen(hold), string+i, 1);
                }
            }else{
                //copy current character to hold
                strncpy(hold+strlen(hold), string+i, 1);
            }
        }

        strcpy(string, hold);
        free(num);
        free(hold);
    }



}

//expands instances of $$ in any command line argument
void expand_all(struct cla* ul){
    int pid = getpid();

    expand_variable(ul->command, pid);
    expand_variable(ul->input, pid);
    expand_variable(ul->output, pid);
    
    for (int i = 0; i < 512; ++i)
        expand_variable(ul->arguments[i], pid);

}

//checks if command is a built in command or not
//if it is built in command returns 1, otherwise 0
int is_built(struct cla* ul){
    if (strcmp(ul->command, "cd") == 0 || strcmp(ul->command, "exit") == 0 || strcmp(ul->command, "status") == 0)
        return 1;
    else
        return 0;
}

//changes directory to either HOME directory, or to argument given
//takes one or no arguments, supports absolute and relative
void built_cd(struct cla* ul){

    //if there is no argument (arguments[0]) to cd, chdir to home
    //if there is an argument (arguments[0]) to cd, chdir to that argument
    if (strcmp(ul->arguments[0], "\0") == 0){
        chdir(getenv("HOME"));
    }else{
        chdir(ul->arguments[0]);
    }

}

//built in function to exit smallsh
void built_exit(){
    kill(0, SIGTERM);
}

//built in function to print status of last command (ignores built in function)
//uses global variables status_exit and status_term
void built_status(){
    //if exit and term both NULL (-1), print exit by 0
    //if term is NULL, print exited value
    //if exit is null, print terminated value
    if (status_exit == -1 && status_term == -1){
        printf("Exited by: 0\n");
        fflush(stdout);
    }else if (status_term == -1){
        printf("Exited by: %d\n", status_exit);
        fflush(stdout);
    }else{
        printf("Terminated by: %d\n", status_term);
        fflush(stdout);
    }
}

//redirects stdin to desired input
void input_redirect(struct cla* ul){
    //if input not null, redirect stdin to input
    //if input null and set to background, redirect stdin to dev/null
    if (strcmp(ul->input, "\0") != 0){
        char in[256];
        memset(in, '\0', 256);
        strcpy(in, ul->input);
        
        //opens input file for reading only
        int inFD = open(in, O_RDONLY, 0644);

        //if can't open file!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (inFD == -1){
            ul->exit_stat = 1;
            perror("Failed to open input file");
            exit(1);
        }

        int direct_in = dup2(inFD, 0);

        //if it can't, error will print and exit status set to 1!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (direct_in == -1){
            ul->exit_stat = 1;
            perror("direct_in");
            exit(2);
        }


    }else{
        if (ul->bg == 1){
            int inFD = open("/dev/null", O_WRONLY);

            int direct_in = dup2(inFD, 0);

            //if it can't, error will print and exit status set to 1!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            if (direct_in == -1){
                ul->exit_stat = 1;
                perror("direct_in");
                exit(2);
            }
        }

    }
}

//redirects stdout to desired output
//can only write to file
void output_redirect(struct cla* ul){
    //if output not null, redirect stdout to output
    //if output null and set to background, redirect stdout to dev/null
    if (strcmp(ul->output, "\0") != 0){
        char out[256];
        memset(out, '\0', 256);
        strcpy(out, ul->output);

        //open output file for writing only, 
        int outFD = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (outFD == -1){
            ul->exit_stat = 1;
            perror("Failed to open file for output");
            exit(1);
        }

        int direct_out = dup2(outFD, 1);

        //if it can't error will print and exit status set to 1!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (direct_out == -1){
            ul->exit_stat = 1;
            perror("failed to redirect output");
            exit(2);
        }

    }else{
        if (ul->bg == 1){
            int outFD = open("/dev/null", O_WRONLY);
            int direct_out = dup2(outFD, 1);

            //if it can't error will print and exit status set to 1!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            if (direct_out == -1){
                ul->exit_stat = 1;
                perror("direct_out");
                exit(2);
            }
        }
    }
   
}

//stores command and arguments and gives to execvp
//redirects stdin and stdout
//executes the given command
void execute_command(struct cla* ul){

    //redirects stdin if input exists
    input_redirect(ul);
    //redirects stdout if output exists
    output_redirect(ul);


    //set up an array that will be passed to execvp
    //initialize all values to be NULL
    char** arr = calloc(514, sizeof(char*));
    for (int i = 0; i < 514; ++i){
        arr[i] = calloc(256, 1);
    } 

    //set first item to the command, and the last element to NULL for execvp
    arr[0] = ul->command;
    arr[513] = NULL; 


    //put any non empty arguments into arr
    for (int i = 1; i <= 512; ++i){
        if (strcmp(ul->arguments[i-1], "\0") != 0){
            arr[i] = ul->arguments[i-1];
        }else{
            arr[i] = NULL;
        }       
    }

    //execute the command
    execvp(arr[0], arr);
    perror("execvp()");
    exit(1);

    //unable to free memory cuz of execute :(
/*     for (int i = 0; i < 514; ++i){
        free(arr[i]);
    }
    free(arr); */


}



//handle SIGTSTP signal
//sends foreground-only mode on or off
void handle_parent_SIGTSTP(int sig){
    //switch fg_only, this acts as a switch toggling fg only either on or off
    if (fg_only)
        fg_only = 0;
    else
        fg_only = 1;
    
    //prints out message accordingly if fg_only is either on or off
    if (fg_only){
        char* message = "\nEntering Foreground-only mode (& is ignored)\n";
        write(STDOUT_FILENO, message, 47);
    }else{
        char* message = "\nExiting Foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
    }
}



int main(){

    //struct cla holds each element of command line, used for parsing
    struct cla ul;
    char* symbol = ": ";

    //allocate memory for struct elements and set to default values
    clear_string(ul.cline);
    allocate_struct(&ul);

    //create orphanage (holds all orphaned processes)
    //initializes all values to be -10 (empty cell)
    struct orphanage orp;
    open_orphanage(&orp);

    //for (int x = 0; x < 10; x++)
    for (;;){
        //ignore SIGINT (parent process + background process)
        signal(SIGINT, SIG_IGN);
        
        //catch signal for TSTP
        struct sigaction SIGTSTP_parent = {{0}};
        SIGTSTP_parent.sa_handler = handle_parent_SIGTSTP;
        sigfillset(&SIGTSTP_parent.sa_mask);
        SIGTSTP_parent.sa_flags = SA_RESTART;
        sigaction(SIGTSTP, &SIGTSTP_parent, NULL);

        //search for pid of background process that are in orphan array if any to print to output when they have finished     
        catch_orphan(&orp, &status_term, &status_exit);

        //print command symbol and get the command line argument into ul
        printf("%s", symbol);
        fflush(stdout);
        fgets(ul.cline, sizeof(ul.cline), stdin);
        ul.empty = no_command(ul.cline);

        //if command not empty, parse for command line inputs
        if (!ul.empty){
            //parse command line 
            //then expand all instances of $$ (this gets pid of parent)
            parse_commandline(ul.cline, ul.command, ul.arguments, ul.input, ul.output, &(ul.bg));
            expand_all(&ul);

            //check if built in command or not, then proceed
             if (is_built(&ul)){
                //set id to -1, so it will command will be identified as built-in
                ul.id = -1;

                if (strcmp(ul.command, "cd") == 0){
                    //executes cd command
                    built_cd(&ul);
                }else if (strcmp(ul.command, "status") == 0){
                    //only execute if previous ul.id not -1
                    built_status();
                }else{
                    //executes exit command
                    built_exit();
                }

            }else{
                //executing process to run in foreground
                if (ul.bg == 0){
                    int child_stat;
                    pid_t child = fork();
                    
                    //fork the child to run in foreground, wait on it on parent
                    switch(child){
                        case -1:
                            perror("fork() foreground\n");
                            exit(1);
                            break;
                        case 0:
                            //ingore SIGTSTP (Doesnt terminate)
                            signal(SIGTSTP, SIG_IGN);

                            //set SIGINT back to default (terminates)
                            signal(SIGINT, SIG_DFL);

                            execute_command(&ul);
                            break;
                        default:   
                            ul.id = child;  //store pid of child
                            child = waitpid(child, &child_stat, 0);

                            //if child got terminated by signal save termination status  
                            if(!WIFEXITED(child_stat)){
                                //if terminated by signal, save signal, turn off exit status, print termination message
                                status_term = WTERMSIG(child_stat);
                                status_exit = -1;
                                printf("Terminated by: %d\n", WTERMSIG(child_stat));
                                fflush(stdout);
                            }else{
                                //if not terminated by signal, save exit status, turn off terminating status
                                status_term = -1;
                                status_exit = WEXITSTATUS(child_stat);                                                             
                            }
                            break;
                    }
                }else{ 
                    //running process in background
                    pid_t child = fork();

                    //fork the background child, store the pid of child in parent
                    switch(child){
                        case -1:
                            perror("fork() background\n");
                            exit(1);
                            break;
                        case 0:
                            //ignore SIGTSTP (doesn't terminate)
                            signal(SIGTSTP, SIG_IGN);

                            //if foreground only mode on, then set SIGINT to default (terminates)
                            if (fg_only == 1)
                                signal(SIGINT, SIG_DFL);

                            //print out pid of child in background only if foreground only mode is off
                            if (fg_only == 0){
                                printf("background pid is %d\n", getpid());
                                fflush(stdout);
                            }

                            execute_command(&ul);
                            break;
                        default:
                            ul.id = child;  //store pid of child

                            //if foreground only mode not on, then store background pid, otherwise wait on child
                            if (fg_only == 0){
                                //find empty space in orphan array and store pid of orphan
                                //if index is -100, that means array is full
                                int index = find_opening(&orp);
                                if (index != -100)
                                    orp.orphans[index] = ul.id;
                            }else{
                                int child_stat;
                                child = waitpid(child, &child_stat, 0);

                                //if child got terminated by signal save termination status  
                                if(!WIFEXITED(child_stat)){
                                    //if terminated by signal, save terminating signal, turn off exit status, print terminating message
                                    status_term = WTERMSIG(child_stat);
                                    status_exit = -1;
                                    printf("Terminated by: %d\n", WTERMSIG(child_stat));
                                    fflush(stdout);
                                }else{
                                    //if not terminated by signal, save exit status, turn off terminating status
                                    status_term = -1;
                                    status_exit = WEXITSTATUS(child_stat);                                                              
                                } 
                            }
                            break;
                    }
                } 


            } 

            //clear all initialized values in cla struct
            clear_cla(&ul);

        }
   
    }

    //free memory in struct
    free(ul.command);
    free(ul.input);
    free(ul.output);
    for (int i = 0; i < 512; ++i)
        free(ul.arguments[i]);
    free(ul.arguments);


    return 0;
}


