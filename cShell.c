#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h> 
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

// Define constants
#define MAX_INPUT_SIZE 1024
#define MAX_ALIAS_SIZE 256
#define MAX_ALIAS_NAME_SIZE 256
#define MAX_ALIAS_COMMAND_SIZE 512
#define MAX_PATH_SIZE 1024
#define MAX_COMMAND_SIZE 128
#define ALIAS_KEY_FILE "aliasKey.txt"
#define ALIAS_COMMAND_FILE "aliasValue.txt"



// Alias struct to store the alias name and command
typedef struct {
    char name[MAX_ALIAS_SIZE];
    char command[MAX_INPUT_SIZE];
} Alias;

// Define global variables
char lastCommand[MAX_INPUT_SIZE];
void execute(char *command , char **args);
void trimInput(char* input);
void getHostname();
void getLastCommand();
void getTTY();
void getHomeLocation();
void getCurrentTimeAndDate();
void getNumProcesses();
void getShellName();
void getUserName();


// Define global variables
Alias aliases[MAX_ALIAS_SIZE];
int aliasCount = 0;

char *trim(char *str) {
    // trim leading and trailing whitespace and quotes
    while (isspace((unsigned char)*str) || *str == '\"') {
        str++;
    }

    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;
    while (end > str && (isspace((unsigned char)*end) || *end == '\"')) {
        end--;
    }

    *(end + 1) = '\0';

    return str;
}

void saveAlias(char *aliasName, char *aliasCommand) {
    // open the alias key and command files
    FILE *keyFile = fopen(ALIAS_KEY_FILE, "a");
    FILE *commandFile = fopen(ALIAS_COMMAND_FILE, "a");
    if (keyFile == NULL || commandFile == NULL) {
        perror("File open failed");
        return;
    }
    // store the alias name and command in the files
    fprintf(keyFile, "%s\n", aliasName);
    fprintf(commandFile, "%s\n", aliasCommand);
    fclose(keyFile);
    fclose(commandFile);
}

int IsAlias(char *aliasName){
    // open the alias key file
    FILE *keyFile = fopen(ALIAS_KEY_FILE, "r");
    // if file open fails print the error and return
    if (keyFile == NULL) {
        perror("File open failed");
        return 0;
    }
    // read the alias key file line by line
    char line[MAX_ALIAS_NAME_SIZE];
    while (fgets(line, sizeof(line), keyFile) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        // if the line is the alias name we are looking for return true
        if(strcmp(line,aliasName) == 0){
            return 1;
        }
    }
    fclose(keyFile);
    return 0;
}

char* getCommand(char *aliasName) {
    // open the alias key and command files
    FILE *keyFile = fopen(ALIAS_KEY_FILE, "r");
    FILE *commandFile = fopen(ALIAS_COMMAND_FILE, "r");

    // if file open fails print the error and return
    if (keyFile == NULL || commandFile == NULL) {
        perror("File open failed");
        return NULL;
    }

    // read the alias key file line by line
    char line[MAX_ALIAS_NAME_SIZE];
    char command[MAX_ALIAS_COMMAND_SIZE];
    int lineNumber = 1;

    // check until the end of the file
    while (fgets(line, sizeof(line), keyFile) != NULL) {
        line[strcspn(line, "\n")] = '\0';
        // if the line is the alias name we are looking for
        if (strcmp(line, aliasName) == 0) {
            // get the command from the command file , match the line number
            fseek(commandFile, 0, SEEK_SET); 
            int i = 1;
            // iterate until the line number matches
            while (fgets(command, sizeof(command), commandFile) != NULL) {
                if (i == lineNumber) {
                    // store and return the command
                    command[strcspn(command, "\n")] = '\0';
                    fclose(keyFile);
                    fclose(commandFile);
                    return strdup(command);
                }
                i++;
            }
        }

        lineNumber++;
    }
    fclose(keyFile);
    fclose(commandFile);
    return NULL;
}
char * getCurrentDirectory(char * cwd){
    // get the last token of the current working directory
    char *token = strtok(cwd, "/");
    char *lastToken = token;
    while (token != NULL) {
        lastToken = token;
        token = strtok(NULL, "/");
    }
    return lastToken;
}

void printPrompt() {
    // Print the shell prompt
    FILE *fp;
    char username[MAX_INPUT_SIZE];
    // get the username
    fp = popen("whoami", "r");
    if (fp == NULL) {
        perror("popen");
        exit(EXIT_FAILURE);
    }

    if (fgets(username, sizeof(username), fp) != NULL) {
        
        username[strcspn(username, "\n")] = '\0';
        // get the hostname
        char hostname[MAX_INPUT_SIZE];
        gethostname(hostname, sizeof(hostname));
        
        // get the current working directory
        char cwd[MAX_INPUT_SIZE];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s@%s %s%s --- ", username, hostname, "~/",getCurrentDirectory(cwd));
        } else {
            // if getcwd fails print the error and exit
            perror("getcwd");
            exit(EXIT_FAILURE);
        }
    } else {
        // if fgets fails print the error and exit
        perror("fgets");
        exit(EXIT_FAILURE);
    }

    pclose(fp);
}



int main() {

    

    char input[MAX_INPUT_SIZE];

    while (1) {
        
        // Print the shell prompt
        printPrompt();

        // Read the input store it in input variable
        if (fgets(input, sizeof(input), stdin) == NULL) {
            perror("Invalid argument");
        }

        input[strcspn(input, "\n")] = '\0';
        // if input is exit break out of the loop
        if (strcmp(input, "exit") == 0) {
            break;
        }
        //if input is bello print the information
        if (strcmp(input, "bello") == 0) {
            getUserName();
            getHostname();
            getLastCommand();
            getTTY();
            getShellName();
            getHomeLocation();
            getCurrentTimeAndDate();
            getNumProcesses();
            continue;
        }

        if (strncmp(input, "cd ", 3) == 0) {
            char *path = input + 3; 
            if (chdir(path) != 0) {
                perror("chdir");
            }
            snprintf(lastCommand, sizeof(lastCommand), "%s", input);
            continue; 
        }
        // store the last command in lastCommand variable
        snprintf(lastCommand, sizeof(lastCommand), "%s", input);
        // Parse the command and arguments
        char *token = strtok(input, " ");
        char *command = token;
        char *args[MAX_INPUT_SIZE];
        args[0] = command;
        // if command is alias parse the alias and store it in the aliases array
        if(strcmp(command,"alias") == 0){
            //if key and value txt not exist create them
            FILE *keyFile = fopen(ALIAS_KEY_FILE, "a");
            FILE *commandFile = fopen(ALIAS_COMMAND_FILE, "a");
            char *aliasName = strtok(NULL, " =\"");
            char *aliasCommand = strtok(NULL, "=");
            // store the alias name in the name field of the alias struct
            snprintf(aliases[aliasCount].name, sizeof(aliases[aliasCount].name), "%s", aliasName);
            if (aliasName == NULL || aliasCommand == NULL) {
                perror("Invalid argument");
                continue;
            }
            if (aliasCount == MAX_ALIAS_SIZE) {
                perror("Too many aliases");
                continue;
            }

            char trimmedCommand[100];
            // trim the alias command
            snprintf(trimmedCommand, sizeof(trimmedCommand), "%s", trim(aliasCommand));
            // store the alias command in the command field of the alias struct
            snprintf(aliases[aliasCount].command, sizeof(aliases[aliasCount].command), "%s", trimmedCommand);

            aliasCount++;
            // save the alias in the file
            saveAlias(aliases[aliasCount - 1].name,  aliases[aliasCount - 1].command);
            fclose(keyFile);
            fclose(commandFile);
            continue;
        }
        // if command is an alias 
        if (IsAlias(command)) {
            // find the alias from the file and get the command
            char *aliasCommand = getCommand(command);
            char *restOfCommand = strtok(NULL, "");
            if (restOfCommand != NULL) {
                // concatanate the alias command and rest of the command
                size_t  newSize = strlen(aliasCommand) + strlen(restOfCommand) + 2;
                char *newCommand = (char *)malloc(newSize);
                if (newCommand != NULL) {
                    // do the rest as if newCommand is a regular command
                    snprintf(newCommand, newSize, "%s %s", aliasCommand, restOfCommand);
                    int arg_count = 0;
                    char *token = strtok(newCommand, " ");
                    char *commandAlias = token;
                    char *aliasArgs[MAX_INPUT_SIZE];
                    aliasArgs[0] = commandAlias;

                    while (token != NULL) {
                        aliasArgs[arg_count++] = token;
                        token = strtok(NULL, " ");
                    }
                    aliasArgs[arg_count] = NULL;

                    execute(commandAlias, aliasArgs);

                    free(newCommand);
                    continue;
                }
            
            }
            else {
                // if there is no rest of the command execute the alias command
                // same as above
                int arg_count = 0;
                char *token = strtok(aliasCommand, " ");
                char *commandAlias = token;
                char *aliasArgs[MAX_INPUT_SIZE];
                aliasArgs[0] = commandAlias;

                while (token != NULL) {
                    aliasArgs[arg_count++] = token;
                    token = strtok(NULL, " ");
                }
                aliasArgs[arg_count] = NULL;
                execute(commandAlias, aliasArgs);
                continue;
                }
        }
    //if command is unalias
    int arg_count = 1;
    while (token != NULL) {
        token = strtok(NULL, " ");
        args[arg_count++] = token;
    }
    args[arg_count - 1] = NULL; 
    execute(command, args);

    
    }
    
    return 0;
}

// reverse the buffer for >>> operation
void reverseBuffer(char *str) {
    int length = strlen(str);
    for (int i = 0; i < length / 2; i++) {
        char temp = str[i];
        str[i] = str[length - i - 1];
        str[length - i - 1] = temp;
    }
}


// execute the command
void execute(char *command, char **args) {
    // define for the redirection type and background process flags
    int background = 0; 
    int redirect_type = 0; 
    char *filename = NULL; 

    // check if the command is a background process
    int i = 0;
    while (args[i] != NULL) {
        if (strcmp(args[i], "&") == 0) {
            background = 1;
            args[i] = NULL;
            break;
        }
        i++;
    }
    
    // fork the process
    pid_t pid = fork();

    // if fork fails print the error and return
    if (pid < 0) {
        perror("Fork failed");
        return;
    } 
    // if pid is 0 it is the child process
    else if (pid == 0) {
        int i = 0;
        // check if there is a redirection
        while (args[i] != NULL) {
            if (strcmp(args[i], ">") == 0 || strcmp(args[i], ">>") == 0 || strcmp(args[i], ">>>") == 0) {
                
                // if there is a redirection get the redirection type , if it is > 0_TRUNC else O_APPEND
                int redirect_type = (strcmp(args[i], ">") == 0) ? O_TRUNC : O_APPEND;

                // get the filename
                char *filename = args[i + 1];
            
                // if there is a >>> operation set the reverse_output flag
                int reverse_output = 0;
                for (int j = 0; args[j] != NULL; j++) {
                    if (strcmp(args[j], ">>>") == 0) {
                        reverse_output = 1;
                        break;
                    }
                }
                
                // open the file with the given filename and redirect type
                int fd = open(filename, O_WRONLY | O_CREAT | redirect_type, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                // duplicate the file descriptor to the stdout
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                // if there is a >>> operation fork the process and reverse the output
                if (reverse_output) {
                    // create a pipe
                    int pipe_fd[2];
                    
                    // if pipe creation fails print the error and exit
                    if (pipe(pipe_fd) == -1) {
                        perror("Pipe creation failed");
                        exit(EXIT_FAILURE);
                    }
                    // fork the process
                    pid_t reverse_pid = fork();

                    // if fork fails print the error and exit
                    if (reverse_pid < 0) {
                        perror("Fork for reverse failed");
                        exit(EXIT_FAILURE);

                    } 
                    // child process
                    else if (reverse_pid == 0) {
                       
                       //close the write end of the pipe
                        close(pipe_fd[1]);

                        // duplicate the read end of the pipe to the standart input
                        dup2(pipe_fd[0], STDIN_FILENO);

                        // close the read end of the pipe and the file descriptor
                        close(fd);
                        close(pipe_fd[0]);

                        // read from the standart input and buffer and reverse it
                        char buffer[1024];
                        size_t bytesRead;
                        while ((bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
                            reverseBuffer(buffer);
                            // then write it to the standart output
                            write(STDOUT_FILENO, buffer, bytesRead);
                        }

                        exit(EXIT_SUCCESS);
                    } else {
                        // parent process
                        // close the read end of the pipe
                        close(pipe_fd[0]);

                       // duplicate the write end of the pipe to the standart output
                        dup2(pipe_fd[1], STDOUT_FILENO);
                        // close the write end of the pipe and the file descriptor
                        close(fd);
                        close(pipe_fd[1]);
                    }
                }
                // close the file descriptor
                close(fd);

                args[i] = NULL;
                args[i + 1] = NULL;
                break;

            }
            i++;
        }
        // if command is clear , clear the screen 
        if (strcmp(command, "clear") == 0) {
            system("clear");
            exit(EXIT_SUCCESS);
        }
        // get the path variable
        char *path = getenv("PATH");
        char *path_copy = strdup(path);
        char *dir = strtok(path_copy, ":");

        // iterate through the directories in the path
        while (dir != NULL) {

            // create the executable path
            char executable[MAX_PATH_SIZE];
            
            // if the command is in the current directory
            snprintf(executable, sizeof(executable), "%s/%s", dir, command);

            // execute the command
            int val = execve(executable, args ,NULL);
            if (val == 0) {
                free(path_copy);
                exit(EXIT_SUCCESS);
            }
            dir = strtok(NULL, ":");
        }

        perror("Command not found");
        free(path_copy);
        exit(EXIT_FAILURE);
    } else {
        // parent process
        // if it is a background process do not wait for the child process
        if (!background) {
            waitpid(pid, NULL, 0);
        }
        return;
    }

}

//get the username
void getUserName(){
    system("whoami");
}
// get the hostname
void getHostname() {
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    printf("%s\n", hostname);
}
// get the last command
void getLastCommand(){
    printf("%s\n",lastCommand);
}
// get the tty
void getTTY() {
    printf("%s\n",ttyname(STDIN_FILENO));
}
// get the shell name
void getShellName() {
    system("echo $SHELL");
}
// get the home location
void getHomeLocation() {
    printf("%s\n", getenv("HOME"));
}
// get the current time and date
void getCurrentTimeAndDate() {
    time_t t;
    time(&t);
    printf("%s", ctime(&t));
}
// get the number of processes
void getNumProcesses() {
    struct sysinfo info;
    sysinfo(&info);
    printf("%d\n", info.procs);
}
