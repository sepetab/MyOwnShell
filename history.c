// COMP1521 18s2 mysh ... command history
// Implements an abstract data object

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "history.h"
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <assert.h>

// This is defined in string.h
// BUT ONLY if you use -std=gnu99
//extern char *strdup(const char *s);

// Command History
// array of command lines
// each is associated with a sequence number

#define MAXHIST 20
#define MAXSTR  200

#define HISTFILE ".mymysh_history"

#define DEBUG 0

typedef struct _history_entry {
    int   seqNumber;
    char *commandLine;
} HistoryEntry;

typedef struct _history_list {
    int nEntries;
    HistoryEntry commands[MAXHIST];
} HistoryList;

HistoryList CommandHistory;

// initCommandHistory()
// - initialise the data structure
// - read from .history if it exists

int initCommandHistory()
{
    // TODO
//Initializing all data tpe in struct array
    for(int i=0;i<20;i++){
        CommandHistory.nEntries=0;
        CommandHistory.commands[i].seqNumber = 0;
//memory assignment to pointer
        CommandHistory.commands[i].commandLine = malloc(MAXSTR);
        strcpy(CommandHistory.commands[i].commandLine, "");
    }
//Stores $home/.mymysh_history from env path
    char *homedir;
    homedir = malloc(2048);
    strcpy(homedir,getenv("HOME"));
    assert(homedir != NULL);
    //printf("homedir = %s", homedir);
    strcat(homedir,"/.mymysh_history");
    //printf("THE PRINT STATEMENT IS: %s\n",homedir);
//Opens history file in homedir for reading    
    FILE *fd = fopen(homedir,"r");
    free(homedir);
    if(fd!=NULL){
        int seqno = 0;
        int return_val = 0;
        char command[MAXSTR];
        char line[500];
        int i =0;
//Gets every line from history file and initializes structs.
        while(fgets(line,500,fd)!=NULL){
            sscanf(line,"%d %[^\t\n]s",&seqno,command);
            if(i<20){
                CommandHistory.commands[i].seqNumber = seqno;
                if(i==0){
                    return_val = seqno;
                }   
                //printf("Sequence number iteration is %d\n",seqno);
                strcpy(CommandHistory.commands[i].commandLine,command);
            }
            i++;
        }
        CommandHistory.nEntries = i;
        fclose(fd);
        return (return_val+1);
    } 
    //returns 1 if anything goes wrong like file cant open         
    return 1;
}

// addToCommandHistory()
// - add a command line to the history list
// - overwrite oldest entry if buffer is full

void addToCommandHistory(char *cmdLine, int seqNo)
{
// TODO
//Moves old commands to next array index to make space for new commandline    
    if(strcmp(CommandHistory.commands[0].commandLine,"")!=0){
        for(int i=19;i>=0;i--){
            if(i-1>=0){
                CommandHistory.commands[i].seqNumber = 
                CommandHistory.commands[i-1].seqNumber;
                strcpy(CommandHistory.commands[i].commandLine,
                CommandHistory.commands[i-1].commandLine);
            }    
        }
    }    
//New command is put into struct here
    strcpy(CommandHistory.commands[0].commandLine,cmdLine);
    CommandHistory.commands[0].seqNumber = seqNo;         

#if DEBUG
    printf("Line aded: %s and next line is %s\n",
    CommandHistory.commands[0].commandLine,CommandHistory.commands[1].commandLine);
    printf("The arguments passed in are %s and %d\n",cmdLine,seqNo);
#endif
}    

// showCommandHistory()
// - display the list of 

void showCommandHistory(/*FILE *outf*/)
{
    // TODO
    //Prints whole struct in required format
    for(int i=19;i>=0;i--){
        //printf("i is %d\n",i);
        if(strcmp(CommandHistory.commands[i].commandLine,"")!=0 ){
            printf(" %3d  %s\n", 
            CommandHistory.commands[i].seqNumber,CommandHistory.commands[i].commandLine);
        }
    }     

}

// getCommandFromHistory()
// - get the command line for specified command
// - returns NULL if no command with this number

char *getCommandFromHistory(int cmdNo)
{
// TODO
//Gets required commandline given cmdNo: iterates through struct    
    for(int i=0;i<20;i++){
        if(CommandHistory.commands[i].seqNumber == cmdNo){
            return CommandHistory.commands[i].commandLine;

        }
    }
    return  NULL;
}
// saveCommandHistory()
// - write history to $HOME/.mymysh_history

void saveCommandHistory()
{
    // TODO
//Stores $home/.mymysh_history path from env
    char *homedir;
    homedir = malloc(2048);
    strcpy(homedir,getenv("HOME"));
    assert(homedir != NULL);
    //printf("homedir = %s", homedir);
    strcat(homedir,"/.mymysh_history");
    //printf("THE PRINT STATEMENT IS: %s\n",homedir);
//opens path to write    
    FILE *fd = fopen(homedir,"w");
    free(homedir);
//Loop writes updated struct in required format    
    for(int i=0; i<20;i++){
        if(CommandHistory.commands[i].commandLine!=NULL){
            if(strcmp(CommandHistory.commands[i].commandLine,"")!=0){
                char temp[500] = "\0";
                fputs(" ",fd);
                fprintf(fd, "%3d", CommandHistory.commands[i].seqNumber);
                fputs("  ",fd);
                strcpy(temp,CommandHistory.commands[i].commandLine);
                strcat(temp,"\n");
                fputs(temp,fd);
            }
        }    
    }
    fclose(fd);
#if DEBUG
    printf("Line saving: %s and with seqno %d\n",
    CommandHistory.commands[0].commandLine,CommandHistory.commands[0].seqNumber);
#endif 
}

// cleanCommandHistory
// - release all data allocated to command history
void cleanCommandHistory()
{
    // TODO
//Frees malloced commandline pointers    
    for(int i=0;i<20;i++){
        if(CommandHistory.commands[i].commandLine!=NULL){
            free(CommandHistory.commands[i].commandLine);
        }
    }     

}



