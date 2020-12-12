#ifndef ORPHANAGE
#define ORPHANAGE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h> 

struct orphanage{
    int orphans[256];   //contains the pid of the orphaned process
};

//initializes all values to -10 in array
void open_orphanage(struct orphanage* orp){
    for (int i = 0; i < 256; ++i)
        orp->orphans[i] = -10;
}

//loops through array to find empty cell (-10)
//if it finds an empty cell it returns that index, otherwise returns (-100) no opening
int find_opening(struct orphanage* orp){
    int open = -100;
    for (int i = 0; i < 256; ++i){
        if (orp->orphans[i] == -10){
            open = i;
            break;
        }
    }
    return open;
}

//clean the cell of the orphan that was removed
void clean_cell(int index, struct orphanage* orp){
    if (orp->orphans[index] != -10){
        orp->orphans[index] = -10;
    }
}

//search for pid of background process
//prints background has finished with waitpid
//catches termination signal
//term_val and exit_val will reference global variables for status
void catch_orphan(struct orphanage* orp, int* term_val, int* exit_val){
    for (int i = 0; i < 256; ++i){
        //if orphans array is filled with a pid save in orp_pid
        if (orp->orphans[i] != -10){
            int orp_pid = orp->orphans[i];
            int orphan_stat;
            //check if it background child has termianted and empty the orphan cell
            if (waitpid(orp_pid, &orphan_stat, WNOHANG) > 0){
                printf("background pid: %d finished\n", orp_pid);
                fflush(stdout);

                //if it didn't exit then save the terminating signal, print it, then make exit_val NULL
                if(!WIFEXITED(orphan_stat)){
                    *(term_val) = WTERMSIG(orphan_stat);
                    (*exit_val) = -1;
                    printf("Terminated by: %d\n", WTERMSIG(orphan_stat));
                    fflush(stdout);
                }else{
                //if it did exit, then save the exit val, print it, then make terminating val NULL
                    *(exit_val) = WEXITSTATUS(orphan_stat);
                    *(term_val) = -1;
                    printf("Exited by: %d\n", WTERMSIG(orphan_stat));
                    fflush(stdout);                 
                }
                clean_cell(i, orp);
            }
        }
    }  
}



#endif