#ifndef __CLASS_1_H
#define __CLASS_1_H

// content
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>

typedef enum {
    COMPILE_SUCCESS,
    COMPILE_UNRECOGNIZED,
    COMPILE_SYNTAX_ERROR
} CompileResult;

typedef enum {
    EXECUTE_TABLE_FULL,
    EXECUTE_SUCCESS,
    EXECUTE_UNDEFINED_ERROR
} ExecuteResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;


#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
typedef struct {
  uint32_t id;
  char username[COLUMN_USERNAME_SIZE];
  char email[COLUMN_EMAIL_SIZE];
} Row;

typedef struct {
    StatementType type;
    char **args; // arguments of the statement
    Row row_to_insert;
} Statement;


#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

void serialize_row(Row* source, void* destination) {
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

const uint32_t PAGE_SIZE = 4096; //4k
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

// takes in a pager number, then return back a block of memory
// all fields except the pages are initialized at struct creation
typedef struct {
    int fd; // file descriptor
    uint32_t file_length;
    void *pages[TABLE_MAX_PAGES];
} Pager;

Pager *pager_open(const char *filename) {
    int fd = open(filename,
     	O_RDWR | 	// Read/Write mode
     	O_CREAT	// Create file if it does not exist
    );
    if(fd == -1){
        fprintf(stderr, "Can't open pager");
        exit(-1);
    }

    off_t file_length = lseek(fd, 0, SEEK_END);
    if(file_length == -1){
        fprintf(stderr, "lseek error");
        exit(-1);
    }
    Pager *pager = (Pager *)malloc(sizeof(Pager));
    pager->fd = fd;
    pager->file_length = file_length;

    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    }

    return pager;
}

void* pager_get_page(Pager *pager, uint32_t page_num){
    if(page_num > TABLE_MAX_PAGES){
        fprintf(stderr, "Page index can't be larger than the maximum");
        exit(-1);
    }

    if(pager->pages[page_num] == NULL){
        void *page = malloc(PAGE_SIZE);
        printf("alloced %p\n", page);

        uint32_t num_pages = pager->file_length / PAGE_SIZE;
        pager->pages[page_num] = page;
        
        if(pager->file_length % PAGE_SIZE > 0){ // file_length is the old one before adding the new page
            num_pages += 1;
        }

        if(page_num > num_pages) {
            fprintf(stderr, "page index out of range");
            exit(-1);
        }

        lseek(pager->fd, page_num*PAGE_SIZE, SEEK_SET);
        ssize_t bytes = read(pager->fd, page, PAGE_SIZE);
        if(bytes == -1){
            fprintf(stderr, "DB read failure");
            exit(-1);
        }

        pager->pages[page_num] = page;
        printf("alloced %p\n", pager->pages[page_num]);
    }

    return pager->pages[page_num];
}

void pager_flush_page(Pager* pager, uint32_t page_num, uint32_t size) {
  if (pager->pages[page_num] == NULL) {
    printf("Tried to flush null page\n");
    exit(EXIT_FAILURE);
  }

  off_t offset = lseek(pager->fd, page_num * PAGE_SIZE, SEEK_SET);

  if (offset == -1) {
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_written = write(pager->fd, pager->pages[page_num], size);

  if (bytes_written == -1) {
    exit(EXIT_FAILURE);
  }
}

typedef struct {
    uint32_t num_rows;
    Pager *pager;
    // void *pages[TABLE_MAX_PAGES];
} Table;

Table *db_open(const char *filename) {
    Pager *pager = pager_open(filename);
    Table *table = (Table *)malloc(sizeof(Table));
    table->num_rows = pager->file_length / ROW_SIZE;
    table->pager = pager;

    return table;
}

void db_close(Table *table){
    uint32_t num_full_pages = table->pager->file_length / PAGE_SIZE;
    for (uint32_t i = 0; i < num_full_pages; i++){
        if(table->pager->pages[i] == NULL){
            continue;
        } 
        pager_flush_page(table->pager, i, PAGE_SIZE);
        free(table->pager->pages[i]);
        table->pager->pages[i] = NULL;
    }
    
    uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
    if (num_additional_rows > 0) {
        uint32_t page_num = num_full_pages;
        if (table->pager->pages[page_num] != NULL) {
            pager_flush_page(table->pager, page_num, num_additional_rows * ROW_SIZE);
            free(table->pager->pages[page_num]);
            table->pager->pages[page_num] = NULL;
        }
    }

    int res = close(table->pager->fd);
    if(res == -1){
        fprintf(stderr, "error closing db");
        exit(-1);
    }

    // free all possible pages defined by TABLE_MAX_PAGES? don't think it will lead to memory leak
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        void *page = table->pager->pages[i];
    }

    free(table->pager);
    free(table);
}

void free_table(Table *table) {
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
        table->pager->pages[i] = NULL;
    }

    free(table);
}


/// @brief where to write the data bytes
/// @param table 
/// @param row_num 
/// @return 
// void *row_slot(Table *table, uint32_t row_num) {
//     uint32_t page_num = row_num / ROWS_PER_PAGE;
    
//     void *page = (void*)pager_get_page(table->pager, page_num);
//     // how many rows are we in the page
//     uint32_t row_offset = row_num % ROWS_PER_PAGE;
//     uint32_t bytes_offset = row_offset * ROW_SIZE; // how many bytes are we `in` the page

//     return page + bytes_offset;
// }

typedef struct {
    Table* table;
    uint32_t row_num;
    bool end_of_table; // Indicates a position one past the last element
} Cursor;

Cursor* table_start(Table* table) {
  Cursor* cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  cursor->row_num = 0;
  cursor->end_of_table = (table->num_rows == 0);

  return cursor;
}

Cursor* table_end(Table* table) {
  Cursor* cursor = malloc(sizeof(Cursor));
  cursor->table = table;
  cursor->row_num = table->num_rows;
  cursor->end_of_table = true;
  return cursor;
}

void* cursor_value(Cursor* cursor) {
  uint32_t row_num = cursor->row_num;
  uint32_t page_num = row_num / ROWS_PER_PAGE;
  void *page = pager_get_page(cursor->table->pager, page_num);
  uint32_t row_offset = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset = row_offset * ROW_SIZE;
  return page + byte_offset;
}

void cursor_advance(Cursor* cursor) {
  cursor->row_num += 1;
  if (cursor->row_num >= cursor->table->num_rows) {
    cursor->end_of_table = true;
  }
 }

void print_row(Row *row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

#endif