/**
 * @file main.c
 * @brief This is the main file for the shell. It contains the main function and the loop that runs the shell.
 * @version 0.1
 * @date 2023-06-02
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <glob.h>

// ---- STRUCTS ----

struct Alias 
{
    char pair[2][100]; // pair[0] = name, pair[1] = value
};

// ---- GLOBAL VARIABLES ----

struct Alias aliases[100]; // array of alias objects
int numOfAliases = 0; // number of alias objects

// ---- FUNCTION DECLARATIONS ---- 

int length(char arr[500][500]); // returns length of a 2D array
void parser(char *inputArr, char parsed[500][500], bool flag); // parses the input string and stores it in parsed array
void inputHandler(char parsed[500][500]); // handles IO redirection and aliases. Calls handleCommand() to execute the command.
bool aliasExists(char *aliasName); //checks if alias exists in array of alias objects
struct Alias* getAlias(char *aliasName); //searches for alias value using name in array of alias objects
void deleteAlias(char *aliasName); //deletes alias from array of alias objects
void addAlias(char *aliasName, char *aliasValue); //adds alias to array of alias objects
void printAliases(); //prints all aliases in array of alias objects
void launchScriptMode(char *fName); //launches the shell in script mode
void launchInteractiveMode(); //launches the shell in interactive mode
int getIndex(char *toFind, char parsed[500][500], int start); //returns the index of a string in a 2D array
void handleCommand(char parsed[500][500]); //handles internal commands and external commands
int countPipes(char parsed[500][500]); //counts the number of pipe symbols in a command
void executePipeline(char parsed[500][500], char command[MAX_STRING_LENGTH]); //executes a pipeline of commands
void getCommands(char command[MAX_STRING_LENGTH], char commands[100][MAX_STRING_LENGTH]); //splits a command into multiple commands based on pipe symbol
bool isValidPipeline(char parsed[500][500], int numOfCommands); //pipeline input validation method
void readFile(char* fName, char lines[100][MAX_STRING_LENGTH], int* lineCount); //reads a file and stores each line in an array
void replaceWildcards(char parsed[500][500]); //replaces wildcard characters with matching filenames

/**
 * @brief This is the main function for the shell. It contains the main loop that runs the shell.
 * 
 * @return int 
 */

int main(int argc, char *argv[100])
{
    if (argc == 2)
    
    {
        launchScriptMode(argv[1]); // launch the shell in script mode with script name as argument.
    }

    if (argc == 1)

    {
        launchInteractiveMode(); // launch the shell in interactive mode
    }

    return 0;
}

int length(char arr[500][500])
{
    int i = 0;
    
    while (arr[i][0] != '\0')
    
    {
        i++;
    }
    
    return i;
}

void parser(char *inputArr, char parsed[500][500], bool flag) 
{
    // if flag is true, drop quotes during parsing, if flag is false, leave quotes in the parsed array

    int index = 0;
    char *traverse, *head, *tail = NULL; // Pointers to the start and end of a token and the current position in the input string
    traverse = inputArr; // Start position pointer from the beginning of the input string

    while (*traverse != '\0') 
    
    {
        while (*traverse == ' ') 
        
        {
            traverse++; // skip leading whitespaces
        }

        head = traverse; // start from the first non-whitespace character
        tail = NULL;

        if (*traverse == '\"' || *traverse == '\'') 
        
        {
            char quote = *traverse;
            traverse++; // move to the character after the quote
            head = traverse; // start from the character after the quote
            tail = strchr(traverse, quote); // find the ending quote
            
            if (tail == NULL)
            
            {
                break; // if no matching quote, break
            }

            if (!flag) 
        
            {
                head--; // include the starting quote in the token
                tail++; // include the ending quote in the token
            }
            
            traverse = tail + 1; // move traverse to the character after the ending quote
        } 
        
        else 
        
        {
            tail = strchr(head, ' '); // find the next whitespace
            
            if (tail == NULL) 
            
            {
                tail = strchr(head, '\0'); // if no whitespace, find the null terminator
            }

            traverse = tail; // nove position pointer  to the next whitespace or null terminator
        }
        
        int length = (tail && head) ? tail - head : 0; // calculate the length of the token (tail - head) if both are not null, otherwise 0
        strncpy(parsed[index], head, length); // copy the token to the parsed array 
        parsed[index][length] = '\0'; // null termination
        
        index++; 
    }
}

bool aliasExists(char *aliasName)
{
    for (int i = 0; i < numOfAliases; i++) // iterate over complete alias objects array to find if alias for cmd exists

    {
        if (strcmp(aliases[i].pair[0], aliasName) == 0) 

        {
            return true;
        }
    }

    return false;
}

void printAliases()
{
    for (int i = 0; i < numOfAliases; i++)

    {
        printf("%s='%s'\n", aliases[i].pair[0], aliases[i].pair[1]);
    }
}

struct Alias* getAlias(char *aliasName)
{
    if (aliasExists(aliasName))

    {
        for (int i = 0; i < numOfAliases; i++)

        {
            if (strcmp(aliases[i].pair[0], aliasName) == 0)

            {
                return &aliases[i];
            }
        }
    }

    return NULL;
}

void deleteAlias(char *aliasName)
{
    for (int i = 0; i < numOfAliases; i++)

    {
        if (strcmp(aliases[i].pair[0], aliasName) == 0)

        {
            for (int j = i; j < numOfAliases - 1; j++)

            {
                aliases[j] = aliases[j + 1]; //shift obj to be deleted to end of array 
            }

            numOfAliases--;

            memset(aliases[numOfAliases].pair[0], '\0', sizeof(aliases[numOfAliases].pair[0])); 
            memset(aliases[numOfAliases].pair[1], '\0', sizeof(aliases[numOfAliases].pair[1]));
        }
    }
}

void addAlias(char *aliasName, char *aliasValue)
{
    if (aliasExists(aliasName))

    {
        LOG_ERROR("Alias already exists!\n");
    }

    else

    {
        strcpy(aliases[numOfAliases].pair[0], aliasName);
        strcpy(aliases[numOfAliases].pair[1], aliasValue);
        numOfAliases++;
    }
}

void readFile(char* fName, char lines[100][MAX_STRING_LENGTH], int* lineCount) 
{
    FILE* file = fopen(fName, "r");
    
    if (!file) 
    
    {
        perror("Error opening file");
        exit(1);
    }

    *lineCount = 0;
    ssize_t read;
    size_t len = 0; // This should be size_t, not int
    char* line = NULL;

    while ((read = getline(&line, &len, file)) != -1) //iterate over the complete file and store each line in an array of inpuuts to execute until no more lines to get.
    
    {
        if (line[read - 1] == '\n') 
        
        {
            line[read - 1] = '\0'; // remove newline character at the end if it exists
        }
        
        strcpy(lines[*lineCount], line); // cpy the read line into the lines array
        
        (*lineCount)++;
    }

    free(line); 
    fclose(file);
}

void launchScriptMode(char *fName) 
{
    using_history();

    char commands[100][MAX_STRING_LENGTH] = {'\0'};
    int numOfCommands = 0;

    readFile(fName, commands, &numOfCommands);

    for (int i = 0; i < numOfCommands; i++)

    {
        char parsed[500][500] = {'\0'};
        
        add_history(commands[i]);
        
        parser(commands[i], parsed, false);

        if (countPipes(parsed) > 0)

        {
            executePipeline(parsed, commands[i]);
        }

        else 

        {
            inputHandler(parsed);
        }
    }
}

void launchInteractiveMode()
{
    using_history(); 

    while (1)

    {
        char *userInput = NULL;
        char inputArr[MAX_STRING_LENGTH] = "";
        char parsed[500][500] = {'\0'};

        userInput = readline("$ ");

        if (userInput == NULL) 
        
        {
            break;   
        }

        else 

        {
            add_history(userInput); // adding the input command to history
         
            strcpy(inputArr, userInput); // copy the input string to arr and free the input buffer 
            free(userInput);
   
            parser(inputArr, parsed, false); // parse the input string and store it in parsed array

            if (countPipes(parsed) > 0)

            {
                executePipeline(parsed, inputArr);
            }

            else 

            {
                inputHandler(parsed);
            }
        }
    }
}

int getIndex(char *toFind, char parsed[500][500], int start)
{
    for (int i = start; i < length(parsed); i++)

    {
        if (strcmp(parsed[i], toFind) == 0)

        {
            return i;
        }
    }

    return -1;
}

int countPipes(char parsed[500][500])
{    
    int count = 0;

    for (int i = 0; i < length(parsed); i++)

    {
        if (strcmp(parsed[i], "|") == 0)

        {
            count++;
        }
    }

    return count;
}

void getCommands(char command[MAX_STRING_LENGTH], char commands[100][MAX_STRING_LENGTH]) 
{
    int index = 0;
    int inQuotes = 0; // flag to check if the current character is inside quotes
    char *start = command; // pointer to the start of a command
    
    for (char *ptr = command; *ptr != '\0'; ptr++) 
    
    {
        if (*ptr == '\"') 
        
        {
            inQuotes = !inQuotes; 
        } 
        
        else if (*ptr == '|' && !inQuotes) 
        
        {
            *ptr = '\0'; // replace the pipe character with null terminator to split the string
            
            while (*start == ' ') 
            
            {
                start++; // Remove leading and trailing whitespaces from the token
            }

            char *end = ptr - 1;
            
            while (end > start && *end == ' ') 
            
            {
                end--;
            }
            
            *(end + 1) = '\0';
            strncpy(commands[index], start, MAX_STRING_LENGTH - 1); // copy the token to the commands array and null terminate it
            commands[index][MAX_STRING_LENGTH - 1] = '\0'; 
            
            start = ptr + 1; // move the start pointer to the next character after the pipe
            index++;
        }
    }
    
    // Handle the last command
    // Remove leading and trailing whitespaces from the token
    while (*start == ' ') start++; // leading
    char *end = start + strlen(start) - 1;
    while (end > start && *end == ' ') end--; // trailing
    *(end + 1) = '\0';
    
    // Copy the last token to the commands array
    strncpy(commands[index], start, MAX_STRING_LENGTH - 1);
    commands[index][MAX_STRING_LENGTH - 1] = '\0'; // Ensure null-termination
}

void executePipeline(char parsed[500][500], char command[MAX_STRING_LENGTH])
{
    const int numOfCommands = countPipes(parsed) + 1;
    int pipes[numOfCommands - 1][2];
    char commandList[100][MAX_STRING_LENGTH] = {"0"}; 
    getCommands(command, commandList);

    if (!isValidPipeline(parsed, numOfCommands)) 
    
    {
        perror("ERR_INVALID_PIPELINE");
        return;
    }

    for (int i = 0; i < numOfCommands - 1; i++) 
    
    {
        pipe(pipes[i]);
    }

    for (int i = 0; i < numOfCommands; i++) 
    
    {
        int rc = fork();
        
        if (rc < 0) 
        
        {
            perror("ERR_FORK_FAILED");
            exit(1);
        } 
        
        if (rc == 0) 
        
        {
            if (i > 0)
            
            {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            
            if (i < numOfCommands - 1)
            
            {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            for (int j = 0; j < numOfCommands - 1; j++)
            
            {
                close(pipes[j][1]);
                close(pipes[j][0]);
            }

            char parsedCommand[500][500] = {"\0"}; 
            parser(commandList[i], parsedCommand, false);
            inputHandler(parsedCommand);
            exit(0);
        } 
    }

    for (int i = 0; i < numOfCommands - 1; i++)
    
    {
        close(pipes[i][0]);
        close(pipes[i][1]);

        if (i < numOfCommands)

        {
            wait(NULL);
        }
    }
}

bool isValidPipeline(char parsed[500][500], int numOfCommands)
{
    int flagwCount = 0;
    int flagaCount = 0;
    int flagrCount = 0;
    int pipeCount = 0;

    for (int i = 0; i < length(parsed); i++)
    
    {
        if (strcmp(parsed[i], ">") == 0)
       
        {
            flagwCount++;
        }
       
        else if (strcmp(parsed[i], ">>") == 0)
       
        {
            flagaCount++;
        }
       
        else if (strcmp(parsed[i], "<") == 0)
       
        {
            flagrCount++;
        }
       
        else if (strcmp(parsed[i], "|") == 0)
       
        {
            pipeCount++;
        }

        if (pipeCount > 0 && pipeCount < numOfCommands - 1 && (strcmp(parsed[i], "<") == 0 || strcmp(parsed[i], ">>") == 0 || strcmp(parsed[i], ">") == 0))
       
        {
            LOG_ERROR("Invalid Pipe Error: Middle commands cannot have IO redirections!\n");
            return false;
        }
    }

    // if the number of pipes is not one less than the number of commands
    if (pipeCount != numOfCommands - 1)
    {
        LOG_ERROR("Invalid Pipe Error: Number of pipes must be one less than the number of commands.!\n");
        return false;
    }

    // if there are multiple input or output redirections
    if (flagwCount > 1 || flagaCount > 1 || flagrCount > 1)
    {
        LOG_ERROR("Invalid Pipe Error: Multiple IO redirection operators!\n");
        return false;
    }

    // if both output redirections are present
    if (flagwCount > 0 && flagaCount > 0)
    {
        LOG_ERROR("Invalid Pipe Error: Both output redirections present!\n");
        return false;
    }

    return true;
}

void handleCommand(char parsed[500][500])
{
    replaceWildcards(parsed);

    char newInput[1000] = "";

    int i = 0;
    
    while (i < 500 && parsed[i][0] != '\0') 
    {
        if (i > 0) 
        
        {
            strcat(newInput, " "); // Add a space between words
        }
        
        strcat(newInput, parsed[i]);
        
        i++;
    }

    parser(newInput, parsed, true); //drop quotes now that we are about to execute command.

    if (strcmp(parsed[0], "exit") == 0)

    {
        exit(0);
    }

    else if (strcmp(parsed[0], "pwd") == 0)
    
    {
        char pwd[MAX_STRING_LENGTH] = "";
        getcwd(pwd, MAX_STRING_LENGTH);
        printf("%s\n", pwd);
    }

    else if (strcmp(parsed[0], "cd") == 0)

    {
        if (length(parsed) > 2) // case where more than 1 argument is given

        {
            LOG_ERROR("Only 2 arguments allowed!\n");
        }

        if (length(parsed) == 1) //case where no argument is given, chdir to home dir 

        {
            chdir("/home");
        }

        else // standard case with 1 argument, chdir to new dir

        {
            chdir(parsed[1]);
        }
    }

    else if (strcmp(parsed[0], "alias") == 0)

    {
        if (length(parsed) == 1) // no args given, list all aliases currently defined

        {
            printAliases();
        }

        else if (length(parsed) == 2) // only alias_name provided, list the expansion of that alias

        {
            if (aliasExists(parsed[1]))

            {
                struct Alias *alias = getAlias(parsed[1]);
                printf("%s='%s'\n", alias->pair[0], alias->pair[1]);
            }

            else

            {
                LOG_ERROR("Alias does not exist!\n");
            }
        }

        else if (length(parsed) == 3)
        
        {
            if (!aliasExists(parsed[1]))

            {
                addAlias(parsed[1], parsed[2]);
            }

            else 

            {
                struct Alias *alias = getAlias(parsed[1]);
                strcpy(alias->pair[1], parsed[2]); //update the value of the existing alias pair.
            }
        }

        else

        {
            LOG_ERROR("Invalid number of arguments!\n");
        }
    }

    else if (strcmp(parsed[0], "unalias") == 0)

    {
        if (aliasExists(parsed[1]))

        {
            deleteAlias(parsed[1]);
        }

        else if (length(parsed) > 2)

        {
            LOG_ERROR("Invalid number of arguments!\n");
        }

        else

        {
            LOG_ERROR("Alias does not exist!\n");
        }
    }

    else if (strcmp(parsed[0], "echo") == 0)

    {
        if (length(parsed) == 1) // no args given, print a new line

        {
            printf("\n");
        }

        else

        {
            for (int i = 1; i < length(parsed); i++)

            {
                printf("%s ", parsed[i]);
            }

            printf("\n");
        }
    }

    else if (strcmp(parsed[0], "history") == 0)

    {
        HIST_ENTRY **list = history_list();
        int numOfHistEntries = history_length;

        if (!list)

        {
            LOG_ERROR("No history!\n");
        }

        else 

        {
            if (length(parsed) == 1) // print all entries added to history.

            {
                for (int i = 0; i < numOfHistEntries; i++)

                {
                    printf("%d %s\n", i + 1, list[i]->line);
                }                
            }

            else if (length(parsed) == 2)

            {
                int numOfEntriesToPrint = atoi(parsed[1]);

                if (numOfEntriesToPrint > numOfHistEntries || numOfEntriesToPrint < 0)

                {
                    LOG_ERROR("Invalid argument value!\n");
                }

                else

                {
                    for (int i = 0; i < numOfEntriesToPrint; i++)

                    {
                        printf("%d %s\n", i + 1, list[i]->line);
                    }
                }
            }

            else

            {
                LOG_ERROR("Invalid number of arguments!\n");
            }
        }
    }

    else

    {
        //handle external commands

        int rc = fork();

        if (rc < 0)

        {
            fprintf(stderr, "fork failed\n");
            exit(1);
        }

        if (rc == 0) // forking a child process to handle execvp 

        {
            char* args[length(parsed) + 1];

            for (int i = 0; i < length(parsed); i++) 

            {
                args[i] = strdup(parsed[i]);
            }

            args[length(parsed)] = NULL;
            
            execvp(args[0], args);

            perror("ERR_EXECVP_FAILED");
            exit(1);
        }

        else

        {
            wait(NULL); // waiting for the child process to finish so the parent process can move forward
        }
    }
}

void inputHandler(char parsed[500][500])
{
    if (aliasExists(parsed[0]))

    {
        struct Alias *alias = getAlias(parsed[0]);
        char newCommand[MAX_STRING_LENGTH] = "";
        strcpy(newCommand, alias->pair[1]);
        
        for (int i = 1; i < length(parsed); i++) 
        
        {
            strcat(newCommand, " ");
            strcat(newCommand, parsed[i]);
        }
        
        parser(newCommand, parsed, false);
    }

    bool write_flag = false;
    bool append_flag = false;
    bool read_flag = false;

    for (int i = 0; i < length(parsed); i++)

    {
        if (strcmp(parsed[i], ">") == 0)

        {
            write_flag = true;
        }

        if (strcmp(parsed[i], ">>") == 0)

        {
            append_flag = true;
        }

        if (strcmp(parsed[i], "<") == 0)

        {
            read_flag = true;
        }
    }

    if (write_flag == false && read_flag == false && append_flag == false)
    
    {
        handleCommand(parsed);
    }

    else

    {
        char* r = "<";
        char* a = ">>";
        char* w = ">";
        int in_backup =  dup(STDIN_FILENO);
        int out_backup = dup(STDOUT_FILENO);

        if (write_flag == true) //rationale: fork each process and call the commandhandler,

        {
            int rc = fork();

            if (rc < 0)

            {
                fprintf(stderr, "fork failed\n");
                exit(1);
            }

            if (rc == 0) // forking a child process to handle execvp 

            {
                int index = getIndex(w, parsed, 0);
                char* fileName = parsed[index + 1];
                int fd = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666); // open file in write only mode, create if it doesn't exist, truncate to replace if it does exist
                dup2(fd, STDOUT_FILENO); // redirect stdout to the file
                close(fd);

                char command[index][500];

                for (int i = 0; i < index; i++)

                {
                    strcpy(command[i], parsed[i]);
                }

                handleCommand(command);
                exit(1);
            }

            else

            {
                wait(NULL); // waiting for the child process to finish so the parent process can move forward
                dup2(out_backup, STDOUT_FILENO);
                close(out_backup);
            }
        }
        
        if (append_flag == true)

        {
            int rc = fork();

            if (rc < 0)

            {
                fprintf(stderr, "fork failed\n");
                exit(1);
            }

            if (rc == 0) // forking a child process to handle execvp 

            {
                int index = getIndex(a, parsed, 0);
                char* fileName = parsed[index + 1];
                int fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0666); // open file in write only mode, create if it doesn't exist, append if it does exist
                dup2(fd, STDOUT_FILENO); // redirect stdout to the file
                close(fd);
                
                char command[index][500];

                for (int i = 0; i < index; i++)

                {
                    strcpy(command[i], parsed[i]);
                }

                handleCommand(command);
                exit(1);
            }

            else

            {
                wait(NULL); // waiting for the child process to finish so the parent process can move forward
                dup2(out_backup, STDOUT_FILENO);
                close(out_backup);
            }
        }

        if (read_flag == true)

        {
            int rc = fork();

            if (rc < 0)

            {
                fprintf(stderr, "fork failed\n");
                exit(1);
            }

            if (rc == 0) // forking a child process to handle execvp 

            {

                int index = getIndex(r, parsed, 0);
                char* fileName = parsed[index + 1];
                //now i need to append each line from the file to the command array after parsing it.

                FILE *f = fopen(fileName, "r");

                if (f == NULL) 

                {
                    perror("ERR_FILE_NOT_FOUND");
                    exit(1);
                }

                char line[MAX_STRING_LENGTH] = "";

                while (fgets(line, sizeof(line), f) != NULL)

                {
                    char command[index][500];

                    for (int i = 0; i < index; i++) 

                    {
                        strcpy(command[i], parsed[i]);
                    }
                    
                    char parsedLine[500][500] = {'\0'};

                    if (line == NULL)

                    {
                        break;
                    }

                    else

                    {
                        if (strlen(line) > 0 && line[strlen(line) - 1] == '\n')
                            
                        {
                            line[strlen(line) - 1] = '\0';
                        }

                        parser(line, parsedLine, false);

                        for (int i = 0; i < length(parsedLine); i++)

                        {
                            strcpy(command[index + i], parsedLine[i]);
                        }

                        handleCommand(command);
                    }
                }
            }

            else

            {
                wait(NULL); // waiting for the child process to finish so the parent process can move forward
                dup2(in_backup, STDIN_FILENO);
                close(in_backup);
            }
        }
    }
}

void replaceWildcards(char parsed[500][500]) 
{
    for (int i = 0; i < 500 && parsed[i][0] != '\0'; i++) 
    
    {
        if (strchr(parsed[i], '*') || strchr(parsed[i], '?')) 
        
        {
            glob_t glob_result;
        
            if (glob(parsed[i], GLOB_TILDE, NULL, &glob_result) == 0) // glob found matches, replace the pattern with the matched filenames
            
            {
                char *replacement = NULL;
                size_t length = 0;
            
                for (unsigned int j = 0; j < glob_result.gl_pathc; j++) 
                
                {
                    length += strlen(glob_result.gl_pathv[j]) + 1; 
                    replacement = realloc(replacement, length + 1); 
                
                    if (j == 0) 
                    
                    {
                        strcpy(replacement, glob_result.gl_pathv[j]);
                    }
                    
                    else
                    
                    {
                        strcat(replacement, " ");
                        strcat(replacement, glob_result.gl_pathv[j]);
                    }
                }

                strcpy(parsed[i], replacement);
                free(replacement);
                globfree(&glob_result);
            } 
            
            else 
            
            {
                globfree(&glob_result); // no matches were found, leave the pattern as is
            }
        }
    }
}
