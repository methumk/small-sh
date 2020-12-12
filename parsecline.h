#ifndef PARSECLINE
#define PARSECLINE

#include <stdlib.h>
#include <stdio.h>
#include <string.h> 

//arguments for the command line
struct cla{
    char cline[2049]; //contains the entire user given command line
    char* command; //contains the command
    char** arguments; //stores the arguments for the command in array of strings
    char* input; //contains the input
    char* output; // contains the output
    char* term_stat; //contains the terminating status from signal
    char* err_stat; //contains message if error happened when dealing with cla

    int bg; //did user specify to run in background
    int empty; //was the command line empty
    int id; //id of the executed command line, (-1) means built in and will be ignored (used for status), (0) for default, otherwise pid
    int exit_stat; //contains exit status number, 0 no error, 1 error
};

//allocate string memory thats been set to null to given size
//returns memory to a string
char* allocate_string(int elem, int size){
    char* hold = (char*)calloc(elem, size);
    return hold;
}

//sets memory of arguments array, sets each argument to null and returns pointer to array
//sets array to size arsize, and each string to ssize
char** allocate_array(int arsize, int ssize){
    char** hold = (char**)calloc(arsize, sizeof(char*));
    for (int i = 0; i < arsize; ++i){
        hold[i] = (char*)calloc(ssize, sizeof(char));
    }
    return hold;
}

//allocates memory and sets to default values for all elements of struct
void allocate_struct(struct cla* ul){
    ul->command = allocate_string(256, 1);
    ul->input = allocate_string(256, 1);
    ul->output = allocate_string(256, 1);
    ul->arguments = allocate_array(512, 256);
    ul->bg = 0;
    ul->empty = 0;
    ul->id = 0;
    ul->exit_stat = 0;
}

//clears each argument in arguments array (only max 512 arguments)
//given that the argument has items in it
void clear_arguments(char** arguments){
    for (int i = 0; i < 512; ++i){
        if (sizeof(arguments[i]) != 0)
            memset(arguments[i],'\0', 256);
    }
}

//clears given string if it is not empty
void clear_string(char* str){
    if (sizeof(str) != 0)
        memset(str,'\0', 256);   
}

//clears all values in CLA struct to default
void clear_cla(struct cla* ul){
    clear_string(ul->command);
    clear_string(ul->input);
    clear_string(ul->output);
    clear_arguments(ul->arguments);
    ul->bg = 0;
    ul->empty = 0;
    ul->id = 0;
    ul->exit_stat = 0;
}

#endif