#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "p5_store.h"

// Test file
#define TEST_DB_FILE "test_db.pansql"

// Helper function to clean up test files before/after tests
void cleanup_test_file() {
    remove(TEST_DB_FILE);
}

// Helper function to find a key in the database
int find_key(FILE* file, uint32_t key) {
    DBHeader header;
    read_header(file, &header);
    uint32_t page_num = header.root_page;
    Page page;
    
    while (1) {
        read_page(file, page_num, &page);
        if (page.is_leaf) {
            // Search for key in leaf page
            for (int i = 0; i < page.num_keys; i++) {
                if (page.keys[i] == key) {
                    return page.values[i]; // Found the key, return its value
                }
            }
            return -1; // Key not found
        }
        
        // Navigate down the tree
        int i;
        for (i = 0; i < page.num_keys; i++) {
            if (key < page.keys[i]) {
                break;
            }
        }
        page_num = page.children[i]; // Go to the next level
    }
}

// Test initialization
void test_init() {
    printf("Running test_init...\n");
    cleanup_test_file();
    
    FILE *file = fopen(TEST_DB_FILE, "wb+");
    assert(file != NULL);
    
    init_db(file);
    
    DBHeader header;
    read_header(file, &header);
    
    assert(header.magic == MAGIC_NUMBER);
    assert(header.version == VERSION);
    assert(header.page_size == PAGE_SIZE);
    assert(header.root_page == 0);
    assert(header.num_page == 1);
    
    Page root;
    read_page(file, 0, &root);
    assert(root.is_leaf == 1);
    assert(root.num_keys == 0);
    
    fclose(file);
    printf("test_init passed!\n");
}

// Test simple insertion
void test_simple_insert() {
    printf("Running test_simple_insert...\n");
    cleanup_test_file();
    
    FILE *file = fopen(TEST_DB_FILE, "wb+");
    assert(file != NULL);
    
    init_db(file);
    
    // Insert a few values
    insert_v2(file, 10, 100);
    insert_v2(file, 5, 50);
    insert_v2(file, 15, 150);
    
    // Read root page
    DBHeader header;
    read_header(file, &header);
    
    Page root;
    read_page(file, header.root_page, &root);
    
    // Check if the insertions were successful
    assert(root.num_keys == 3);
    
    // Verify the order and values
    assert(root.keys[0] == 5);
    assert(root.values[0] == 50);
    assert(root.keys[1] == 10);
    assert(root.values[1] == 100);
    assert(root.keys[2] == 15);
    assert(root.values[2] == 150);
    
    fclose(file);
    printf("test_simple_insert passed!\n");
}

// Test tree splitting
void test_tree_split() {
    printf("Running test_tree_split...\n");
    cleanup_test_file();
    
    FILE *file = fopen(TEST_DB_FILE, "wb+");
    assert(file != NULL);
    
    init_db(file);
    
    // Insert enough values to force splitting
    for (int i = 0; i < MAXIMUM_KEYS + 5; i++) {
        insert_v2(file, i, i * 10);
    }
    
    // Read header to get root page
    DBHeader header;
    read_header(file, &header);
    
    // Verify that the tree has split (root should not be a leaf anymore)
    Page root;
    read_page(file, header.root_page, &root);
    assert(root.is_leaf == 0);
    assert(root.num_keys > 0);
    read_header(file, &header);
    
    // Check that all the inserted values can be found
    for (int i = 0; i < MAXIMUM_KEYS + 5; i++) {
        int value = find_key(file, i);
        assert(value == i * 10);
    }
    
    fclose(file);
    printf("test_tree_split passed!\n");
}

// Test deletion
void test_delete() {
    printf("Running test_delete...\n");
    cleanup_test_file();
    
    FILE *file = fopen(TEST_DB_FILE, "wb+");
    assert(file != NULL);
    
    init_db(file);
    DBHeader header;
    read_header(file, &header);
    
    // Insert values
    for (int i = 0; i < 10; i++) {
        insert_v2(file, i, i * 10);
    }
    read_header(file, &header);
    
    // Delete a few values
    destroy_v2(file, header.root_page, 3);
    read_header(file, &header);
    destroy_v2(file, header.root_page, 7);

    
    // Check that deleted values are no longer in the tree
    assert(find_key(file, 3) == -1);
    assert(find_key(file, 7) == -1);
    
    // Check that non-deleted values are still there
    for (int i = 0; i < 10; i++) {
        if (i != 3 && i != 7) {
            assert(find_key(file, i) == i * 10);
        }
    }
    
    fclose(file);
    printf("test_delete passed!\n");
}

// Test complex operations (insert, delete, find)
void test_complex_operations() {
    printf("Running test_complex_operations...\n");
    cleanup_test_file();
    
    FILE *file = fopen(TEST_DB_FILE, "wb+");
    assert(file != NULL);
    
    init_db(file);
    
    // Insert values in random order
    int values[] = {50, 25, 75, 12, 37, 62, 87, 6, 18, 31, 43, 56, 68, 81, 93};
    int num_values = sizeof(values) / sizeof(values[0]);
    
    for (int i = 0; i < num_values; i++) {
        insert_v2(file, values[i], values[i] * 2);
    }
    
    // Verify all values are present
    for (int i = 0; i < num_values; i++) {
        assert(find_key(file, values[i]) == values[i] * 2);
    }
    DBHeader header;
    read_header(file, &header);
    
    // Delete some values
    destroy_v2(file, header.root_page, 25);
    read_header(file, &header);
    destroy_v2(file, header.root_page, 62);
    read_header(file, &header);
    destroy_v2(file, header.root_page, 93);
    
    // Verify deleted values are gone
    assert(find_key(file, 25) == -1);
    assert(find_key(file, 62) == -1);
    assert(find_key(file, 93) == -1);
    
    // Verify remaining values
    assert(find_key(file, 50) == 50 * 2);
    assert(find_key(file, 12) == 12 * 2);
    assert(find_key(file, 75) == 75 * 2);
    
    // Insert new values
    insert_v2(file, 100, 1000);
    insert_v2(file, 25, 250);  // Reinsert a deleted value
    
    // Check the new insertions
    assert(find_key(file, 100) == 1000);
    assert(find_key(file, 25) == 250);
    
    fclose(file);
    printf("test_complex_operations passed!\n");
}

// Main test runner
int main() {
    printf("Starting B-tree unit tests...\n");
    
    test_init();
    test_simple_insert();
    test_tree_split();
    test_delete();
    test_complex_operations();
    
    cleanup_test_file();
    printf("All tests passed successfully!\n");
    return 0;
}
