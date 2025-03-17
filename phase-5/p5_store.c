#include "idx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAGIC_NUMBER 0x50414E53514C  // "PANSQL" in hex
#define VERSION 1
#define DB_FILENAME "db.pansql"
#define DEGREE 3
#define MAXIMUM_KEYS ((DEGREE * 2) - 1)
#define MID ((MAXIMUM_KEYS) / 2)
#define MINIMUM_KEYS (DEGREE - 1)
#define PAGE_SIZE sizeof(Page)

typedef struct {
    uint64_t magic;     // Magic number (8 bytes)
    uint32_t version;   // Schema version (4 bytes)
    uint32_t page_size; // Page size (4 bytes)
    uint32_t num_page; // total number of pages
    uint32_t root_page; // Root page index (4 bytes)
} DBHeader;

typedef struct Page {
    uint8_t is_leaf;
    uint32_t page_num;
    uint32_t num_keys;          // Number of keys stored
    uint32_t parent_page_num;
    uint32_t keys[MAXIMUM_KEYS];    // Fixed-size key storage
    union {
	uint32_t values[MAXIMUM_KEYS];  // Fixed-size value storage
	uint32_t children[MAXIMUM_KEYS + 1];  // Fixed-size value storage
    };

} Page;

void insert_into_leaf(FILE *file, uint32_t page_num, uint32_t key, uint32_t value);

void write_page(FILE *file, uint32_t page_number, Page *page) {
    fseek(file, sizeof(DBHeader) + page_number * sizeof(Page), SEEK_SET);
    fwrite(page, sizeof(Page), 1, file);
    fflush(file);
}

void read_page(FILE *file, uint32_t page_number, Page *page) {
    fseek(file, sizeof(DBHeader) + page_number * sizeof(Page), SEEK_SET);
    fread(page, sizeof(Page), 1, file);
}

void read_header(FILE *file, DBHeader *db_header) {
    fseek(file, 0, SEEK_SET);
    fread(db_header, sizeof(DBHeader), 1, file);
}

void write_header(FILE *file, DBHeader *db_header) {
    fseek(file, 0, SEEK_SET);
    fwrite(db_header, sizeof(DBHeader), 1, file);
    fflush(file);
}

void init_page(Page *page, uint32_t page_num, uint8_t is_leaf) {
    memset(page, 0, sizeof(Page));  // Zero everything
    page->page_num = page_num; // Store page number inside struct
    page->is_leaf = is_leaf;
    page->num_keys = 0; // No keys initially
    page->parent_page_num = UINT32_MAX;
}

void init_db(FILE *file) {

    if (!file) {
        perror("Failed to create database file");
        exit(1);
    }

    DBHeader header = {
        .magic = MAGIC_NUMBER,
        .version = VERSION,
        .page_size = PAGE_SIZE,
        .root_page = 0, // Initially, no root page
        .num_page = 1 // Initially, no root page
    };

    Page root;
    init_page(&root, 0, 1);

    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(DBHeader), 1, file);
    fflush(file);

    write_page(file, 0, &root);

    printf("Database initialized: %s\n", DB_FILENAME);
}

void traverse(FILE *file, uint32_t page_num, int depth) {
    Page page;
    read_page(file, page_num, &page);

    if(page.is_leaf) {
		for (int i = 0; i < page.num_keys; i++) {
			printf("Depth %d: Page %i| index = %d | key %i | Value: %i\n", depth, page.page_num, i, page.keys[i], page.values[i]);
		}
    } else {
		traverse(file, page.children[0], depth + 1);
		printf("Depth %d: : Page %i| numkeys %i| index = %d | key %i \n", depth, page.num_keys,page.page_num, 0, page.keys[0]);
		for (int i = 1; i <= page.num_keys; i++) {
		    traverse(file, page.children[i], depth + 1);
		    if (i < page.num_keys) printf("Depth %d: Page %i| index = %d | key %i \n", depth, page.page_num, i, page.keys[i]);
		}

    }
}

uint32_t allocate_new_page(FILE *file) {
   DBHeader header;

    // Read DB header
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(DBHeader), 1, file);

    uint32_t new_page_num = header.num_page; // Get next available page
    header.num_page++; // Increment total pages

    // Write updated header back to disk
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(DBHeader), 1, file);
    fflush(file);

    return new_page_num;
}

uint32_t create_new_page(FILE *file, uint8_t is_leaf) {
    uint32_t page_num = allocate_new_page(file); // Step 1: Allocate page number

    Page page;
    init_page(&page, page_num, is_leaf);  // Step 2: Initialize page in memory
    write_page(file, page_num, &page);  // Step 3: Write page to disk

    return page_num;
}

int insert_into_page(Page *page, uint32_t key, uint32_t value) {
    if (page->num_keys >= MAXIMUM_KEYS) return -1; // Page is full
    
    page->keys[page->num_keys] = key;
    page->values[page->num_keys] = value;
    page->num_keys++;
    return 0; // Success
}

void insert_v2(FILE *file, uint32_t key, uint32_t value) {
    // seek the correct leaf first
    DBHeader header;
    read_header(file, &header);
    uint32_t page_num = header.root_page;
    Page page;
    while (1) {
        read_page(file, page_num, &page);
        if (page.is_leaf) {
            break;
        }
        // Find the correct child pointer
        int i;
        for (i = 0; i < page.num_keys; i++) {
            if (key < page.keys[i]) {
                break;
            }
        }
        page_num = page.children[i];  // Go down the correct child
    }
    insert_into_leaf(file, page_num, key, value);
}

int destroy_v2(FILE *file, uint32_t page_num, uint32_t key) {
    // seek the correct leaf first
    Page page;
    read_page(file, page_num, &page);

    if(!page.is_leaf) {
	// uint32_t child_page_num = // logic for finding child_index
        int i;
        for (i = 0; i < page.num_keys; i++) {
            if (key < page.keys[i]) {
                break;
            }
        }
        uint32_t child_page_num = page.children[i];  // Go down the correct child
	int new_key = destroy_v2(file, child_page_num, key);

	Page child_page;
	read_page(file, child_page_num, &child_page);

	if (new_key >= 0 && child_page_num > 0) {
	    child_page.keys[child_page_num - 1] = new_key;
	    write_page(file, child_page_num, &child_page);
	    new_key = -1;
	}
	if (child_page.num_keys < MINIMUM_KEYS) {
	    bool borrowed = false;
	    Page left_child, right_child, moved_grandchild;
	    if(i < page.num_keys) {
		read_page(file, page.children[i+1], &right_child);

		if (right_child.num_keys > MINIMUM_KEYS) {
		    if(!right_child.is_leaf) {

			child_page.children[child_page.num_keys + 1] = right_child.children[0];
			read_page(file, right_child.children[0], &moved_grandchild);
			moved_grandchild.parent_page_num = child_page_num;

			child_page.keys[child_page.num_keys] = page.keys[i];
			page.keys[i] = right_child.keys[0];

			int j = 1;
			// shift right child
			while (j < right_child.num_keys) {
			    right_child.keys[j - 1] = right_child.keys[j];
			    right_child.children[j-1] = right_child.children[j];
			    j++;
			}
			right_child.children[j - 1] = right_child.children[j];
			write_page(file, moved_grandchild.page_num, &moved_grandchild);

		    } else {
			// claim right values to borrower
			child_page.values[child_page.num_keys] = right_child.values[0];
			// in this case right_child.keys[0] == node.keys[childIndex]
			child_page.keys[child_page.num_keys] = right_child.keys[0];
			// claim first key of right as the new separator
			page.keys[i] = right_child.keys[1];
			// shift right values
			int j = 1;
			while (j < right_child.num_keys) {
			    right_child.keys[j - 1] = right_child.keys[j];
			    right_child.values[j-1] = right_child.values[j];
			    j++;
			}
		    }
		    right_child.num_keys--;
		    child_page.num_keys++;
		    write_page(file, page.children[i+1], &right_child);
		    write_page(file, page.children[i], &child_page);
		    write_page(file, page.page_num, &page);
		    borrowed = true;
		}
	    }
	    if(!borrowed && i > 0 ) {
		read_page(file, page.children[i-1], &left_child);
		int j = child_page.num_keys;

		if(left_child.num_keys > MINIMUM_KEYS) {
		    if(!left_child.is_leaf) {
			child_page.children[j+1] = child_page.children[j];
			while (j > 0) {
			    child_page.keys[j] = child_page.keys[j-1];
			    child_page.children[j] = child_page.children[j-1];
			    j--;
			}
			// claim left
			child_page.keys[0] = page.keys[i-1];
			child_page.children[0] = left_child.children[left_child.num_keys];

			read_page(file, child_page.children[0], &moved_grandchild);
			moved_grandchild.parent_page_num = child_page.page_num;
			//claim foremost right children of left
			page.keys[i-1] = left_child.keys[left_child.num_keys -1];

			write_page(file, moved_grandchild.page_num, &moved_grandchild);
		    } else {
			while (i > 0) {
			    child_page.keys[i] = child_page.keys[i-1];
			    child_page.values[i] = child_page.values[i-1];
			    i--;
			}
			child_page.keys[0] = left_child.keys[left_child.num_keys-1];
			child_page.values[0] = left_child.values[left_child.num_keys-1];
			page.keys[i-1] = child_page.keys[0];
		    }
		    left_child.num_keys--;
		    child_page.num_keys++;
		    write_page(file, page.children[i-1], &left_child);
		    write_page(file, page.children[i], &child_page);
		    write_page(file, page.page_num, &page);
		    borrowed = true;
		}
	    }

	    // merge with right if right is minimum
	    if(!borrowed && i < page.num_keys) {
		read_page(file, page.children[i+1], &right_child);
		if(!right_child.is_leaf) {
			child_page.keys[child_page.num_keys] = page.keys[i];
			for(int j = 0; j < right_child.num_keys; j++) {
			    read_page(file, right_child.children[j], &moved_grandchild);
			    child_page.keys[child_page.num_keys + 1 + j] = right_child.keys[j];
			    child_page.children[child_page.num_keys + 1 + j] = right_child.children[j]; // this stops at child_page.num
			    moved_grandchild.parent_page_num = child_page.page_num; //update parent on merge
			    write_page(file, right_child.children[j], &moved_grandchild);
			}
			child_page.children[child_page.num_keys + right_child.num_keys + 1] = right_child.children[right_child.num_keys];

			read_page(file, right_child.children[right_child.num_keys], &moved_grandchild);
			moved_grandchild.parent_page_num = child_page.page_num; //update parent on merge
			write_page(file, right_child.children[right_child.num_keys], &moved_grandchild);
			child_page.num_keys += right_child.num_keys + 1;

		} else {
			for(int j = 0; j < right_child.num_keys; j++) {
			    child_page.keys[child_page.num_keys + j] = right_child.keys[j];
			    child_page.values[child_page.num_keys + j] = right_child.values[j];
			}
			child_page.num_keys += right_child.num_keys;
		}
		for(int k = i; k < page.num_keys - 1; k++) {
			page.keys[k] = page.keys[k+1];
			page.children[k+1] = page.children[k+2];
		}
		page.num_keys--;
		// right child is now free, need to add free page list
		write_page(file, page_num, &page);
		write_page(file, child_page_num, &child_page);
		borrowed = true;
	    }
	    if(!borrowed && i > 0) {
		read_page(file, page.children[i-1], &left_child);

		if(!left_child.is_leaf) {
			left_child.keys[left_child.num_keys] = page.keys[i-1];
			for(int m = 0; m < child_page.num_keys; m++) {
				left_child.keys[left_child.num_keys + 1 + m] = child_page.keys[m];
				left_child.children[left_child.num_keys + 1 + m] = child_page.children[m]; // this stops at child_page.num
				read_page(file, child_page.children[m], &moved_grandchild);
				moved_grandchild.parent_page_num = left_child.page_num; //update parent on merge
				write_page(file, child_page.children[m], &moved_grandchild);
			}
			left_child.children[child_page.num_keys + left_child.num_keys + 1] = child_page.children[child_page.num_keys];
			read_page(file, child_page.children[child_page.num_keys], &moved_grandchild);
			moved_grandchild.parent_page_num = left_child.page_num; //update parent on merge
			write_page(file, child_page.children[right_child.num_keys], &moved_grandchild);
			left_child.num_keys += child_page.num_keys + 1;
		} else {
			for(int k = 0; k < child_page.num_keys; k++) {
				left_child.keys[left_child.num_keys + k] = child_page.keys[k];
				left_child.values[left_child.num_keys + k] = child_page.values[k];
			}
			left_child.num_keys += child_page.num_keys;
		}

		for(int j = i; j < page.num_keys; j++) {
			page.keys[j-1] = page.keys[j];
			page.children[j] = page.children[j+1];
		}
		page.num_keys--;

		// child_page is now free, should add to freelist
		write_page(file, page_num, &page);
		write_page(file, page.children[i-1], &left_child);
		borrowed = true;
	    }

	    if(page.parent_page_num == UINT32_MAX && page.num_keys == 0) {
		read_page(file, page.children[0], &child_page);
		child_page.parent_page_num = UINT32_MAX;
		// child_page is set as root
		write_page(file, page.children[0], &child_page);
		// page is now free, should add to freelist
		DBHeader db_header;
		read_header(file, &db_header);
		db_header.root_page = page.children[0];
		write_header(file, &db_header);
	    }
	}
	// code for splitting merging etc
	if(new_key >= 0 && child_page_num <= 0) {
	    return new_key;
	} else {
	    // assign new_key to index
	    return -1;
	}
    } else {
	int left = 0;
	int right = page.num_keys - 1;

	if (key < page.keys[left]) {
	    printf("Key %d not found\n", key);
	    return -1;
	}

	int mid;

	if (key > page.keys[right]) {
	    printf("Key %d not found\n", key);
	    return -1;
	}
	while(left <= right) {
	    mid = left + ((right - left)  / 2 );

	    if (page.keys[mid] == key) {
		    break;
	    } else if (page.keys[mid] < key) {
		    left = mid + 1;
	    } else {
		    right = mid - 1;
	    }
	}
	if (page.keys[mid] == key) {
	    int i = mid;
	    while(i < page.num_keys-1) {
		page.keys[i] = page.keys[i+1];
		page.values[i] = page.values[i+1];
		i++;
	    }
	    page.num_keys--;
	    write_page(file, page.page_num, &page);

	    if(mid == 0) {
		// promote new key for its parent
		return page.keys[0];
	    }
	    return -1;
	} else {
	    printf("Key %d not found, mid: %i\n", key, mid);
	    return -1; // early exit due to not found
	}
    }
}

void promote_to_parent_page(FILE *file, uint32_t page_num, uint32_t promoted_key, uint32_t promoted_page_num) {
    Page page;
    Page promoted_page;
    read_page(file, page_num, &page);
    read_page(file, promoted_page_num, &promoted_page);
    DBHeader db_header;
    read_header(file, &db_header);

    if(page.parent_page_num == UINT32_MAX) {
	// reading header DB for getting root page
	uint32_t root_page_num = db_header.root_page;

	Page root;
	read_page(file, root_page_num, &root);

	Page new_root;
	uint32_t new_root_page_num = create_new_page(file, 0);
	read_header(file, &db_header);
	read_page(file, new_root_page_num , &new_root);
	db_header.root_page = new_root_page_num;
	db_header.num_page++; // instead of re-read the header, just add here

	new_root.keys[0] = promoted_key;
	new_root.children[0] = page.page_num;
	new_root.children[1] = promoted_page.page_num;
	new_root.num_keys = 1;
	page.parent_page_num = new_root_page_num;
	promoted_page.parent_page_num  = new_root_page_num;

	write_header(file, &db_header);

	write_page(file, new_root.page_num, &new_root);
	write_page(file, page.page_num, &page);
	write_page(file, promoted_page.page_num, &promoted_page);

    } else {
	printf("start messing up after here:\n");
	Page parent;
	read_page(file, page.parent_page_num, &parent);

	int i = parent.num_keys;
	while (i > 0 && parent.keys[i - 1] > promoted_key) {
	    parent.keys[i] = parent.keys[i - 1];
	    parent.children[i + 1] = parent.children[i];
	    i--;
	}

	parent.keys[i] = promoted_key;
	parent.children[i + 1] = promoted_page.page_num;
	promoted_page.parent_page_num = page.parent_page_num;
	parent.num_keys++;

	if(parent.num_keys >= MAXIMUM_KEYS) {    
	    Page second_half;
	    uint32_t second_half_page_num = create_new_page(file, 0);
	    read_page(file, second_half_page_num, &second_half);
	    
	    int new_promoted = parent.keys[MID];
	    
	    second_half.children[0] = parent.children[MID + 1];

	    // update child parent page num
	    Page child;
	    read_page(file, second_half.children[0], &child);
	    child.parent_page_num = second_half.page_num; 
	    write_page(file, child.page_num, &child);
	    
	    for (int j = MID + 1; j < parent.num_keys; j++) {
		second_half.keys[j - (MID + 1)] = parent.keys[j];
		second_half.children[j - MID] = parent.children[j + 1];
		read_page(file, second_half.children[j - MID], &child);
		child.parent_page_num = second_half.page_num; 
		write_page(file, child.page_num, &child);
	    }

	    if (i <= MID) {
		promoted_page.parent_page_num = parent.page_num;
	    } else {
		promoted_page.parent_page_num = second_half.page_num;
	    }
	    write_page(file, promoted_page.page_num, &promoted_page);
	    
	    second_half.num_keys = parent.num_keys - (MID + 1);
	    parent.num_keys = MID;
	    
	    second_half.parent_page_num = parent.parent_page_num;
	    write_page(file, parent.page_num, &parent);
	    write_page(file, second_half.page_num, &second_half);
			
	    promote_to_parent_page(file, parent.page_num, new_promoted, second_half.page_num);
	} else {
	    write_page(file, promoted_page.page_num, &promoted_page);
	    write_page(file, parent.page_num, &parent);
	}
    }
}

void insert_into_leaf(FILE *file, uint32_t page_num, uint32_t key, uint32_t value) {
	// need to handle splitting if overflow
    DBHeader db_header;
    read_header(file, &db_header);
    Page page;
    read_page(file, page_num, &page);
    int i = page.num_keys;
    while (i > 0 && page.keys[i-1] > key) {
	page.keys[i] = page.keys[i-1];
	page.values[i] = page.values[i-1];
	i--;
    }
    page.keys[i] = key;
    page.values[i] = value;
    page.num_keys++;

    if(page.num_keys >= MAXIMUM_KEYS) {
	Page new_page;
	uint32_t new_page_num = create_new_page(file, 1);
	read_page(file, new_page_num, &new_page);
	int promoted_key = page.keys[MID];

	for(int k = MID; k < page.num_keys; k++) {
	    new_page.keys[k-MID] = page.keys[k];
	    new_page.values[k-MID] = page.values[k];
	}
	new_page.num_keys = page.num_keys - MID;
	page.num_keys  = MID;
	new_page.parent_page_num = page.parent_page_num;
	write_page(file, new_page_num, &new_page);
	write_page(file, page.page_num, &page);

	promote_to_parent_page(file, page.page_num, promoted_key, new_page_num);

    } else {
	write_page(file, page_num, &page);
    }
}

int main () {
    FILE *file = fopen(DB_FILENAME, "rb+");
    DBHeader db_header;
    if (!file) {
	printf("Database file not found. Creating a new one...\n");
	file = fopen(DB_FILENAME, "wb+");
	if (!file) {
	    perror("Failed to create database file");
	    return 1;
	}
	init_db(file);
    } else {
	read_header(file, &db_header);
	if (db_header.magic != MAGIC_NUMBER) {
	    printf("Invalid database file! Reinitializing...\n");
	    fclose(file);
	    file = fopen(DB_FILENAME, "wb+");
	    if (!file) {
		perror("Failed to recreate database file");
		return 1;
	    }
	    init_db(file);
	} else {
	    printf("Valid database file found. Magic number: 0x%lX\n", MAGIC_NUMBER);
	    printf("Valid database file found. Version: %i\n", db_header.version);
	    printf("Valid database file found. Page Size: %i\n", db_header.page_size);
	    
	}

    }

    for(int i = 0; i < 24; i++) {
	read_header(file, &db_header);
	insert_v2(file, i, i);
    }

    printf("=======\n");
    read_header(file, &db_header);
    Page pagehi;
    read_page(file, db_header.root_page, &pagehi);

    traverse(file, db_header.root_page, 0);
    
    fclose(file);
    return 0;

}
