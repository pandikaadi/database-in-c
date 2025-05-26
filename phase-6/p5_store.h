#ifndef BTREE_H
#define BTREE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define MAGIC_NUMBER 0x50414E53514C  // "PANSQL" in hex
#define VERSION 1
#define DB_FILENAME "db.pansql"
#define DEGREE 3
#define MAXIMUM_KEYS ((DEGREE * 2) - 1)
#define MID ((MAXIMUM_KEYS) / 2)
#define MINIMUM_KEYS (DEGREE - 1)
#define PAGE_SIZE sizeof(Page)
#define MAX_COLUMNS 8
#define MAX_ROW_SIZE 256
#define MAX_COLUMN_NAME 32

typedef enum {
    TYPE_UINT,
    TYPE_STRING
} ColumnType;

typedef struct {
    char name[MAX_COLUMN_NAME];
    ColumnType type;
    uint32_t size;
    uint32_t offset;
} Column;

typedef struct {
    uint32_t num_columns;
    Column columns[MAX_COLUMNS];
    uint32_t row_size;
} Schema;

typedef struct {
    uint64_t magic;     // Magic number (8 bytes)
    uint32_t schema_version;   // Schema version (4 bytes)
    uint32_t page_size; // Page size (4 bytes)
    uint32_t num_page;  // total number of pages
    uint32_t root_page; // Root page index (4 bytes)
    uint32_t free_list; // free list pages
} DBHeader;

typedef struct Page {
    uint8_t is_leaf;
    uint32_t page_num;
    uint32_t schema_version;
    uint32_t num_keys;          // Number of keys stored
    uint32_t parent_page_num;
    uint32_t prev;
    uint32_t next;
    uint32_t keys[MAXIMUM_KEYS];    // Fixed-size key storage
    union {
        uint8_t values[MAXIMUM_KEYS][MAX_ROW_SIZE];  // Fixed-size value storage
        uint32_t children[MAXIMUM_KEYS + 1];  // Fixed-size value storage
    };
} Page;

// Function declarations
void range_key_query(FILE *file, uint32_t smallest, uint32_t biggest, int asc);
void init_db(FILE *file);
void insert_v2(FILE *file, void **values);
int destroy_v2(FILE *file, uint32_t page_num, uint32_t key);
void read_header(FILE *file, DBHeader *db_header);
void write_header(FILE *file, DBHeader *db_header);
void read_page(FILE *file, uint32_t page_number, Page *page);
void write_page(FILE *file, uint32_t page_number, Page *page);
void traverse(FILE *file, uint32_t page_num, int depth);

#endif /* BTREE_H */
