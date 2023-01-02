#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "structs.h"

#define DEFAULT_BUF_SIZE 100

void wldb_loop(void);
char *wldb_read_line(void);
char **wldb_split_line(char *line);
int wldb_process_statement(char **args);
CompileResult wldb_compile(char **args, Statement *statement);

int main(int argc, char **argv){
    // step1: load and execute configs

    wldb_loop();

    return 0;
}

void wldb_loop(void){
    char *line;
    char **args; // a pointer to a pointer to a series of characters
    int status;

    do {
        printf("> ");
        line = wldb_read_line();
        args = wldb_split_line(line);
        status = wldb_process_statement(args);
        
        free(line);
        free(args);
    } while(status == 0);

    return;
}

char *wldb_read_line(void){
    int bufsize = DEFAULT_BUF_SIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * DEFAULT_BUF_SIZE);
    int c;

    if(!buffer){
        fprintf(stderr, "Buffer allocation failure\n");
        exit(-1);
    }   

    // this can be replaced with getline()
    while(1){
        c = getchar();
        //c = fgetc(stdin);

        if(c == EOF){
            printf("Shell exiting~");
            exit(0);
        } else if(c == '\n'){ 
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
            position++;
        }

        if(position >= bufsize){
            bufsize += DEFAULT_BUF_SIZE;
            buffer = realloc(buffer, bufsize);
            if(!buffer){
                fprintf(stderr, "Buffer reallocation failure\n");
                exit(-1);
            }
        }
    }
}


#define TOK_BUFSIZE 10      // allow 10 arguments
#define TOK_DELIM " \t\r\n\a"
char **wldb_split_line(char *line) {
    int bufsize = DEFAULT_BUF_SIZE;
    int position = 0;
    char **tokens = malloc(sizeof(char *) * bufsize); // allow bufsize number of arguments
    char *token; // each argument 

    if(!tokens){
        fprintf(stderr, "Can't allocate for tok buf\n");
        exit(-1);
    }

    token = strtok(line, TOK_DELIM);
    while(token){
        tokens[position] = token;
        position++;

        if(position >= bufsize){
            bufsize += TOK_BUFSIZE;
            tokens = realloc(tokens, bufsize);
            if(!tokens){
                fprintf(stderr, "realloc failed");
                exit(-1);
            }
        }
        token = strtok(NULL, TOK_DELIM);
    }
    tokens[position] = NULL; // so the returned pointer array is null-eneded.
    return tokens;
}

int wldb_execute(Statement statement){

    return 0;
}

int wldb_cd(char **args);
int wldb_exit(char **args);

char *builtin_funcs_str[] = {
    "cd",
    "exit"
};

// list of function pointers
int (*builtin_funcs[]) (char **) = {
    &wldb_cd,
    &wldb_exit
};

int wldb_num_builtins() {
    return sizeof(builtin_funcs_str) / sizeof(char *);
}

int wldb_cd(char **args){
    if(args[1] == NULL) {
        fprintf(stderr, "No arg given to cd\n");
        exit(-1);
    }
    else {
        if(chdir(args[1]) != 0) {
            perror("Cd");
        }
    }
    return 0;
}

int wldb_exit(char **args) {
    return -1; 
}

int wldb_process_statement(char **args){
    int num_builtins = wldb_num_builtins();
    if(strncmp(args[0], ".", 1) == 0) { // it's a meta command
        for(int i = 0; i < num_builtins; i++) {
            if(strcmp(builtin_funcs_str[i], args[0]+1) == 0) {
                return (*builtin_funcs[i])(args);
            }
        }
        fprintf(stderr, "Unrecognized meta command\n");
        return 0;
    }

    Statement statement;
    switch(wldb_compile(args, &statement)) {
        case COMPILE_SUCCESS:
            break;
        case COMPILE_UNRECOGNIZED:
            fprintf(stderr, "Unrecognized SQL command\n");
            return 0;
        case COMPILE_FAILURE:
            fprintf(stderr, "Compile failure\n");
            return -1;
    }

    return wldb_execute(statement);
}

int wldb_select(char **args);
int wldb_insert(char **args);

/// @brief built-in SQL commands
char *sql_cmd_strs[] = {
    "select",
    "insert"
};

/// @brief list of function pointers
int (*sql_cmd_funcs[]) (char **) = {
    &wldb_select,
    &wldb_insert
};

int wldb_num_sql_builtins() {
    return sizeof(sql_cmd_strs) / sizeof(char *);
}

int wldb_select(char **args) {
    printf("selected\n");
    return 0;
}

int wldb_insert(char **args) {
    printf("inserted\n");
    return 0;
}

CompileResult wldb_compile(char **args, Statement *statement) {
    // int num_sql_builtins = wldb_num_sql_builtins();
    // for(int i = 0; i < num_sql_builtins; i++) {
    //     if(strcmp(sql_cmd_strs[i], args[0]) == 0) {
    //         return (*sql_cmd_funcs[i])(args);
    //     }   
    // }

    if(strcmp(args[0], "select") == 0) {
        statement->type = STATEMENT_SELECT;
        statement->args = args;
        return COMPILE_SUCCESS;
    }
    if(strcmp(args[0], "insert") == 0) {
        statement->type = STATEMENT_INSERT;
        statement->args = args;
        return COMPILE_SUCCESS;
    }

    fprintf(stderr, "Unrecognized SQL command\n");
    return 0;
}