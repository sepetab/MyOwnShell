// mysh.c ... a small shell
// Started by John Shepherd, September 2018
// Completed by Aravind Venkateswaran, September/October 2018

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glob.h>
#include <assert.h>
#include <fcntl.h>
#include "history.h"
#define DEBUG 0
#define DEBUGGLOB 1

// This is defined in string.h
// BUT ONLY if you use -std=gnu99
//extern char *strdup(char *);

// Global Constants
#define MAXLINE 200
#define BUFSIZE 100000

// Global Data
//global that determines whether to be added to history or not
int saveornot = 0;
//Lets the program know if there are any globbed items mainly for cd
int globbed =0;

// Function prototypes
void trim(char *);
int strContains(char *, char *);
char **tokenise(char *, char *);
char **fileNameExpand(char **);
void freeTokens(char **);
char *findExecutable(char *, char **);
int isExecutable(char *);
void prompt(void);
void execute(char **args, char **path, char **envp, char pathcopy[BUFSIZE]);






// Main program
// Set up enviroment and then run main loop
// - read command, execute command, repeat

int main(int argc, char *argv[], char *envp[])
{
    pid_t pid;   // pid of child process
    int stat;    // return status of child
    char **path; // array of directory names
    //int cmdNo;   // command number
    int i;       // generic index

// set up command PATH from environment variable
    for (i = 0; envp[i] != NULL; i++) {
        if (strncmp(envp[i], "PATH=", 5) == 0) break;
    }
    if (envp[i] == NULL)
        path = tokenise("/bin:/usr/bin",":");
    else
// &envp[i][5] skips over "PATH=" prefix
        path = tokenise(&envp[i][5],":");
#ifdef DBUG
    for (i = 0; path[i] != NULL;i++)
        printf("path[%d] = %s\n",i,path[i]);
#endif

// initialise command history
// - use content of ~/.mymysh_history file if it exists

    int cmdNo = initCommandHistory();
    //printf("CURRENT CMD NO IS %d",cmdNo);
// main loop: print prompt, read line, execute command
    char line[MAXLINE];
    prompt();
//gets command line
    while (fgets(line, MAXLINE, stdin)!=NULL) {
//Re-initializes globbed as zero
        globbed =0;
        trim(line); // remove leading/trailing space
//If line Null print prompt and rerun
        if (line[0] == '\0') {
            prompt();
            continue;
        }
//Save or not re-initialized to zero        
        saveornot=0;
        int checker =0;
        char convert[220] = "";
        int seqnumber = 0;
        if(line[0] == '!'){
                     
//If line starts with !, check syntax and return required output.        
            int j=0;//string index which holds required command number
            for(int i=1;i<201 && line[i]!='\0';i++){
//Command checks for !!
                if(line[1]=='!' /*&& line[2] == '\0'*/){
                    seqnumber = cmdNo-1; 
                    checker =1;
                }else if((line[i]<'0' || line[i]>'9')){
                    printf("Invalid history substitution\n");
                    seqnumber = -1;//Set to -1 if invalid
                    saveornot =0;
                    break;
                }else{
                    convert[j] = line[i];
                    j++;

                }
            }
            if(line[1]=='\0'){
                printf("Invalid history substitution\n");
                    seqnumber = -1;//Set to -1 if invalid
                    saveornot =0;
            }       
            //printf("The convert string is %s\n",convert);
            if(seqnumber==-1){
                prompt();continue;//If invalid then rerun entire loop
            }    
            if(seqnumber==0 && checker ==0){
                seqnumber = atoi(convert);//since line is characters
            }  
            if(seqnumber>=cmdNo || seqnumber==0 || seqnumber<=cmdNo-21){
                printf("No command #%d\n",seqnumber);prompt();continue;
            }else{
//Line is substituted with required commmand line            
                //printf("The needed seqno is %d\n",seqnumber);
                strcpy(line,getCommandFromHistory(seqnumber));
                printf("%s\n",line);
            }  
        }
//args to store final tokenized and (if) globbed line   
//args2 to stor initialize tokenized line       
        char **args;
        char **args2;
        args2 = tokenise(line," ");
        args = fileNameExpand(args2);    
        // TODO
//If exit is first argument, exit shell         
        if(strcmp(args[0],"exit")==0){
            printf("\n");
            exit(0);
//If command is history, showhistory called            
        }else if(strcmp(args[0],"h")==0 || strcmp(args[0],"history")==0){
            showCommandHistory(); 
            saveornot=1; 
//Pwd to print out current directory using getcwd            
        }else if(strcmp(args[0],"pwd")==0){
            char cwd[BUFSIZE];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                printf("%s\n", cwd);
            }else{
                printf("Error in getcwd");
            }
            saveornot=1; 
//Cd to change directory
        }else if(strcmp(args[0],"cd")==0){
            char cwd[BUFSIZE];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                //printf("%s\n", cwd);
            }else{
                printf("Error in getcwd\n");
            }
            if(args[1]!=NULL){
                saveornot=1;
                char *dir;
                char *to;
                char cwds[BUFSIZE];
                char compare[BUFSIZE];
                strcpy(compare,cwd);
                dir = strcat(cwd,"/");
                to = strcat(dir,args[1]);
                if(globbed){
                    chdir(args[1]); // If globbed, entire path is in args[1]
                }else{    
                    //printf("newdir: %s\n", to);
                    chdir(to);
                }    
                if (getcwd(cwds, sizeof(cwds)) != NULL) {
                    //printf("%s\n", cwd);
                }else{
                    printf("Error in getcwd\n");
                }

                if(strcmp(cwds,compare)==0 && strcmp(args[1],".")!=0){
                    saveornot=0;
                    printf("%s: No such file or directory\n",args[1]);
                }
                if(saveornot!=0){    
                    printf("%s\n",cwds);
                }    
                //printf("cwd:%s & cwds:%s\n",cwd,cwds);

            }else{
//If just cd entered, change to homedir
                char *homedir;
                char cwds[BUFSIZE];
                homedir = malloc(2048);
                strcpy(homedir,getenv("HOME"));
                assert(homedir != NULL);
                chdir(homedir);
                free(homedir);
                if (getcwd(cwds, sizeof(cwds)) != NULL) {
                    printf("%s\n", cwds);
                }else{
                    printf("Error in getcwd\n");
                }                              
                saveornot =1;
            }
//If no shell builtins or ! expansion, then the command is executed
        }else{   
            pid = fork();
            if (pid == 0){
//Stores index at which ">" or "<" is located
                int redirectindex = -1;
//flag is set to 1 if invalid redirection or command not found                
                int flag =0;
//Checks for redirections                
                for(int i=0;args[i]!=NULL;i++){
                    if(strcmp(args[i],"<")==0||strcmp(args[i],">")==0){
                        redirectindex = i;
                        break;
                    } 
                }
                //printf("Redirectindex is %d\n",redirectindex);
//Checks for executable paths                
                char filename[BUFSIZ]="";
                char pathcopy[BUFSIZ]="";
                if(args[0][0] == '/' || args[0][0] == '.'){
                    if(isExecutable(args[0])){
                        strcpy(filename,args[0]);                
                        strcat(pathcopy,args[0]);
                    }    
                }else{
                    for(int i =0;path[i]!=NULL;i++){
                        strcpy(pathcopy,path[i]);
                        strcat(pathcopy, "/");
                        strcat(pathcopy, args[0]);
                        if(isExecutable(pathcopy)){
                            strcpy(filename,args[0]);
                            break;
                        }                        
                    }            
                }
//No command found if no paths found                
                if(strcmp(filename,"")==0 && strcmp(args[0],">")!=0 && 
                strcmp(args[0],"<")!=0){
                    printf("%s: Command not found\n",args[0]);
                    flag =1;
                }
//redirection is tackled here
                if(redirectindex!=-1){
//If its "<" input                
                    if(strcmp(args[redirectindex],"<")==0){
                        //printf("The args is %s and %s",
                        //args[redirectindex + 1],args[redirectindex + 2]);
//Statement checks for all invalid inputs                        
                        if(redirectindex==0||args[redirectindex + 1] ==
                           NULL || args[redirectindex + 2] !=NULL || 
                                strcmp(args[redirectindex + 1],">")==0 || 
                                strcmp(args[redirectindex + 1],"<")==0){
                            printf("Invalid i/o redirection\n");
                            flag =1;
                        }
//If valid, Then file is opened and stdin is substituted using dup2 to file                        
                        if(flag ==0){
                            int infd;
                            infd = open(args[redirectindex+1],O_RDONLY);
                            if (infd>=0){
                                printf("Running %s ...\n",pathcopy);
                                printf("--------------------\n");
                                dup2(infd,0);
                                free(args[redirectindex]);
                                free(args[redirectindex+1]);
                                args[redirectindex]=NULL;
                                
                            }else{
                                perror("Input redirection");
                                flag =1;
                            }     
                            close(infd);    
                        }
//Same tactic is implemented for output redirection                           
                    }else{               
                        if(redirectindex==0||args[redirectindex + 1] ==NULL 
                           || args[redirectindex + 2] !=NULL || 
                           strcmp(args[redirectindex + 1],">")==0 || 
                           strcmp(args[redirectindex + 1],"<")==0){
                            printf("Invalid i/o redirection\n");
                            flag =1;
                        }
                        //printf("The flag in output redirection is %d\n",flag);
                        if(flag ==0){
                            int outfd;
                            outfd = open(args[redirectindex+1],
                            O_WRONLY|O_CREAT|O_TRUNC, 0644);
                            if(outfd>=0){
                                printf("Running %s ...\n",pathcopy);
                                printf("--------------------\n");
                                dup2(outfd,1);
                                free(args[redirectindex]);
                                free(args[redirectindex+1]);
                                args[redirectindex]=NULL;
                                close(outfd);
                            }else{
                                perror("Output redirection");
                                flag =1;
                                close(outfd);
                            }    
                                
                        }    
                    }
                }
//The running is only displayed when no errors are there and no < or >                 
                if(redirectindex==-1 && flag ==0){
                    printf("Running %s ...\n",pathcopy);
                    printf("--------------------\n");
                }
//No errors, then execute
                if(flag ==0){     
                    execute(args,path,envp,pathcopy); 
                }else{
                    return 100;
                }        
            }else if(pid !=-1) {
                wait(&stat);
                stat = stat/256;
                //printf("Stat here is %d\n",stat);
//If flag is set to 1, 100 is returned in child and if this is the case, 
//then dont print return value                
                if(stat!=100){
                    printf("--------------------\n");
                    printf("returns %d\n",stat);
                }    
//If execution is smooth, then saved or else discarded
                if(stat!=0){
                    saveornot = 0;
                }else{
                    saveornot =1;
                }       
            }else{
                printf("Error in fork");
            }
        }
        //addded to history
        if(saveornot==1){
            addToCommandHistory(line,cmdNo);
            cmdNo++;
        }
        // TODO
        //free tokens and save to history
        freeTokens(args); 
        saveCommandHistory(); 
        prompt();
    }
    //cleaned at end of shell
    cleanCommandHistory();
    printf("\n");
    return(EXIT_SUCCESS);
}

void execute(char **args, char **path, char **envp,char pathcopy[BUFSIZE])
{
//Execution occurs in this function
    if(execve(pathcopy,args,envp)==-1){
        printf("Error in execve\n");
    }         
}

// fileNameExpand: expand any wildcards in command-line args
// - returns a possibly larger set of tokens
char **fileNameExpand(char **tokens)
{
//printf("It is being executed here\n");
    // TODO
//Will contain Final tokenized set of strings    
    char **returns = malloc((200)*sizeof(char*));
//To keep track of returns index    
    int returns_index=0;
//1 if special characters are found  
    int checker=0;
//Loop iterates through every argument    
    for(int i=0;tokens[i]!=NULL;i++){
        checker=0;
//Loop to check if any of the argument contains special characters                
        for(int j=0;tokens[i][j]!='\0';j++){
            if(tokens[i][j] == '[' || tokens[i][j] == '?' || 
               tokens[i][j] == '~' || tokens[i][j] == '*'){
                checker=1;//set to 1 if any argument has speial characters
                //printf("Checker set to 1");
                glob_t *myglob = malloc(sizeof(glob_t));
                if((glob(tokens[i],GLOB_NOCHECK|GLOB_TILDE,NULL,myglob)!=0)){
//unchanged argument stored in returns if no globbed strings were found               
                    returns[returns_index] = strdup(tokens[i]); 
                    returns_index++;
                }else{
                    for(int k=0;myglob->gl_pathv[k]!=NULL;k++){
//or else globbed strings are stored in returns                    
                        //printf("gl_pathv's are : %s\n",myglob->gl_pathv[i]);
                        returns[returns_index] = strdup(myglob->gl_pathv[k]);
                        returns_index++;
                        globbed=1;//global variable changed
                        //printf("GLOBBING TAKING PLACE\n");
                    }

                }                
                free(myglob);               
//break ensures to only do this routine once if an argument with special character is found 
            }
            if(checker == 1){
                break;
            }    
        }
//If no special character, store unchanged argument        
        if(checker == 0){
            returns[returns_index] = strdup(tokens[i]);
            returns_index++;
        }    
    }
//Final character is set to NULL    
    returns[returns_index]=NULL;
    returns_index++;
//Original tokens freed    
    freeTokens(tokens);
//New tokens returned    
    return returns;    
}

// findExecutable: look for executable in PATH
char *findExecutable(char *cmd, char **path)
{
    char executable[MAXLINE];
    executable[0] = '\0';
    if (cmd[0] == '/' || cmd[0] == '.') {
        strcpy(executable, cmd);
        if (!isExecutable(executable))
            executable[0] = '\0';
    }
    else {
        int i;
        for (i = 0; path[i] != NULL; i++) {
            sprintf(executable, "%s/%s", path[i], cmd);
            if (isExecutable(executable)) break;
        }
        if (path[i] == NULL) executable[0] = '\0';
    }
    if (executable[0] == '\0')
        return NULL;
    else
        return strdup(executable);
}

// isExecutable: check whether this process can execute a file
int isExecutable(char *cmd)
{
    struct stat s;
    // must be accessible
    if (stat(cmd, &s) < 0)
        return 0;
    // must be a regular file
    //if (!(s.st_mode & S_IFREG))
    if (!S_ISREG(s.st_mode))
        return 0;
    // if it's owner executable by us, ok
    if (s.st_uid == getuid() && s.st_mode & S_IXUSR)
        return 1;
    // if it's group executable by us, ok
    if (s.st_gid == getgid() && s.st_mode & S_IXGRP)
        return 1;
    // if it's other executable by us, ok
    if (s.st_mode & S_IXOTH)
        return 1;
    return 0;
}

// tokenise: split a string around a set of separators
// create an array of separate strings
// final array element contains NULL
char **tokenise(char *str, char *sep)
{
    // temp copy of string, because strtok() mangles it
    char *tmp;
    // count tokens
    tmp = strdup(str);
    int n = 0;
    strtok(tmp, sep); n++;
    while (strtok(NULL, sep) != NULL) n++;
    free(tmp);
    // allocate array for argv strings
    char **strings = malloc((n+1)*sizeof(char *));
    assert(strings != NULL);
    // now tokenise and fill array
    tmp = strdup(str);
    char *next; int i = 0;
    next = strtok(tmp, sep);
    strings[i++] = strdup(next);
    while ((next = strtok(NULL,sep)) != NULL)
        strings[i++] = strdup(next);
    strings[i] = NULL;
    free(tmp);
    return strings;
}

// freeTokens: free memory associated with array of tokens
void freeTokens(char **toks)
{
    for (int i = 0; toks[i] != NULL; i++)
        free(toks[i]);
    free(toks);
}

// trim: remove leading/trailing spaces from a string
void trim(char *str)
{
    int first, last;
    first = 0;
    while (isspace(str[first])) first++;
    last  = strlen(str)-1;
    while (isspace(str[last])) last--;
    int i, j = 0;
    for (i = first; i <= last; i++) str[j++] = str[i];
    str[j] = '\0';
}

// strContains: does the first string contain any char from 2nd string?
int strContains(char *str, char *chars)
{
    for (char *s = str; *s != '\0'; s++) {
        for (char *c = chars; *c != '\0'; c++) {
            if (*s == *c) return 1;
        }
    }
    return 0;
}

// prompt: print a shell prompt
// done as a function to allow switching to $PS1
void prompt(void)
{
    printf("mymysh$ ");
}


