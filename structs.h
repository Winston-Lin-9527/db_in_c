typedef enum {
    COMPILE_SUCCESS,
    COMPILE_UNRECOGNIZED,
    COMPILE_FAILURE
} CompileResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef struct {
    StatementType type;
    char **args; // arguments of the statement
} Statement;

typedef struct {
    
} Row;

#define COLUMN_USERNAME_SIZE