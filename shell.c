// David Stekol and Serene Joe
// HW 3 Poblem 3
// ECE-357
// 10/21/2022

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <grp.h>
#include <sys/wait.h>
#include <sys/sysmacros.h>
#include <stdbool.h>
#include <sys/time.h> 
#include <sys/resource.h> 

int wstatus;

struct commandArray {
    char str[1024][1025];
    char* command;
    char* argument = "";
    char redirTokens[1024][1025];
    int count=0;
    int IO;
    int fds[3];  
};


struct commandArray lineParse(char *line){
    struct commandArray temp;
    int i = 0;

    char* token = strtok(line, " ");
    char *p1;
    int setIndex;
    char pathname[1024];
    
    temp.IO = 0;
    

    while (token != NULL) {

        if(i == 0){
            temp.command = token;
        }
        if(i == 1){
            temp.argument = token;
        }

        if ((p1=strstr(token, ">")) != NULL || (p1=strstr(token, "<")) != NULL){

            setIndex = p1 - token;
            temp.IO = temp.IO + 1;

            //case 1
            if (token[setIndex] == '<'){
                strcpy(temp.redirTokens[temp.IO-1], token);
            }
            //case 2
            if (token[setIndex] == '>'){
                strcpy(temp.redirTokens[temp.IO-1], token);
                //printf("2\n");
            }

            
            //case 3
            else if (token[setIndex + 1] != '>'){
                strcpy(temp.redirTokens[temp.IO-1], token);
                //printf("3\n");
            }

            //case 4 + 5
            else if (token[setIndex +2] != '>'){
                strcpy(temp.redirTokens[temp.IO-1], token);    
               // printf("4\n");         
            }
            

        }
        if ((p1=strstr(token, ">")) == NULL && (p1=strstr(token, "<")) == NULL){
           if(i != 0){
            strcpy(temp.str[i-1], token);
            
            temp.count = temp.count + 1; 
           } 
           
        }
        

        
        i++;
        token = strtok(NULL, " ");
    }

   
    return temp;    
}
 

int executeCommands(struct commandArray test){

    char* command;
    char* argument;
    

    command = test.command;
    argument = test.argument;

    char* lastUsedCommand;

    //char fullArg[1024];

    char s[100];
    

    if (strcmp(command, "cd") == 0 ){
        if(strcmp(argument, "") != 0){
            chdir(argument);
        }
        
        if(strcmp(argument, "") == 0){
            chdir(getenv("HOME"));
            }
            return 0;
        }
    
    if (strcmp(command, "pwd") == 0 ){
        printf("%s\n", getcwd(s, PATH_MAX + 1));
        return 0;
    }
    
    if (strcmp(command, "exit") == 0 ){
        if(strcmp(argument, "") != 0){
            exit(atoi(argument));
            }
        
        if(strcmp(argument, "") == 0){
            exit(WEXITSTATUS(wstatus));
        }      

    }
    
    if (fork() == 0){

        char* p1;
        char* token;
        char pathname[1024];
        int setIndex;
        
        for (int i = 0; i<test.IO; i++){
            
            token = test.redirTokens[i];
            if ((p1=strstr(token, ">")) != NULL || (p1=strstr(token, "<"))){
                setIndex = p1 - token;
                if (token[setIndex] == '<'){
                    memcpy(pathname, &token[setIndex + 1], strlen(token) - 1 );
                    //Open filename and redirect stdin
                    test.fds[i] = open(pathname, O_CREAT|O_RDONLY, 0777);
                    dup2(test.fds[i], 0);
                }
                
                //case 2 + 3
                else if (token[setIndex + 1] != '>'){
                    
                    memcpy(pathname, &token[setIndex + 1], strlen(token) - 1 );
                   // printf("Pathname : %s\n", pathname);
                    test.fds[i] = open(pathname, O_CREAT|O_TRUNC|O_WRONLY, 0777);
                   // printf("File descriptor is %i\n", test.fds[i]);
                    if(token[0] == '>'){
                        dup2(test.fds[i],1);
                    }
                    else{
                        dup2(test.fds[i],2);
                    }
                }

                //case 4 + 5
                else if (token[setIndex +2] != '>'){
                
                    memcpy(pathname, &token[setIndex + 2], strlen(token) - 1 );
                    test.fds[i] = open(pathname, O_CREAT|O_APPEND| O_WRONLY, 0777);
                    if(token[0] == '>'){
                        dup2(test.fds[i],1);
                    }
                    else{
                        dup2(test.fds[i],2);
                    }
                }
            }   
        }

        //EXECING OF STUFF HERE
       //printf("Test count is :%i\n", test.count);
        if(test.count > 0){
            char* fullArg[test.count];
            fullArg[0] = command;
            for (int j = 0; j<test.count; j++){
            //printf("Argument %i is %s \n", j+1, test.str[j]);
            
            //strcat(fullArg, "\"");
            fullArg[j+1] = test.str[j];
            }
            execvp(command,fullArg); 
        }
        else{
            execlp(command, command, NULL);
        }
        exit(127);
    }

    
    else{
        wait(&wstatus);
    }
    return 0;
    }

//COMMANDS THAT WE NEED: cd, pwd, exit, ls, cat, echo
int main(int argc, char* argv[]){
    char* fileName;
    FILE *defaultStream = stdin;
    int placeholderfd;
    if (argc == 2){
        fileName = argv[1];
        char* placeholderMode = "r";
        placeholderfd = open(fileName, O_RDONLY);
        defaultStream = fdopen(placeholderfd, placeholderMode);
    }
    char *line = NULL;
    size_t len = 0;
    ssize_t lineSize = 0;
    struct commandArray store;
    int i = 0;
    
//    executeCommands();
    while((lineSize = getline(&line, &len, defaultStream)) >= 0 ){
        if (lineSize == 0 || line[0] == '#')
            continue;
        line[lineSize-1] = '\0';
        store = lineParse(line);
        executeCommands(store);
    }
    return 0;
}
