#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h> // for random
#include "p5_store.h"


void insert_into_leaf(FILE *file, uint32_t page_num, uint32_t key, void **values);

void get_schema(Schema *schema) {
    schema->num_columns = 2;

    schema->columns[0].type = TYPE_UINT;
    schema->columns[0].size = sizeof(uint32_t);
    strcpy(schema->columns[0].name, "id");

    schema->columns[1].type = TYPE_STRING;
    schema->columns[1].size = 32;
    strcpy(schema->columns[1].name, "username");

    schema->row_size = 0;
    for (int i = 0; i < schema->num_columns; i++) {
	schema->columns[i].offset = schema->row_size;
	schema->row_size += schema->columns[i].size;
    }

}

void build_row(uint8_t *buffer, const Schema *schema, void **values) {
    for (int i = 0; i < schema->num_columns; i++) {
	Column col = schema->columns[i];
	void *src = values[i];
	memcpy(buffer + col.offset, src, col.size);
    }
}

void print_row(uint8_t *buffer, const Schema *schema) {
    for (int i = 0; i < schema->num_columns; i++) {
	Column col = schema->columns[i];
	printf("| %s:", col.name);
	if (col.type == TYPE_UINT) {
	    uint32_t val;
	    memcpy(&val, buffer + col.offset, sizeof(uint32_t));
	    printf("%d |", val);
	} else if (col.type == TYPE_STRING) {
	    printf("%s |", (char *) buffer + col.offset);
	}
    }
    printf("\n");

}

int binary_search_internal_node(Page *page, uint32_t key) {
    int left = 0;
    int right = page->num_keys;

    while (left < right) {
	int mid = left + ( (right - left) / 2);

	if (key < page->keys[mid]) {
	    right = mid;
	} else {
	    left = mid + 1;
	}
    }
    return left;
}

int binary_search_leaf(Page *page, uint32_t key) {
    int left = 0;
    int right = page->num_keys;
    while ( left < right ) {
	int mid = left + ( (right - left) / 2 );

	if (page->keys[mid] < key) {
	    left = mid + 1;
	} else {
	    right = mid;
	}     
    }
    return left;
}

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

void init_page(Page *page, uint32_t page_num, uint8_t is_leaf, uint32_t schema_version) {
    memset(page, 0, sizeof(Page));  // Zero everything
    page->page_num = page_num; // Store page number inside struct
    page->is_leaf = is_leaf;
    page->num_keys = 0; // No keys initially
    page->parent_page_num = UINT32_MAX;
    page->prev = UINT32_MAX;
    page->next = UINT32_MAX;
    page->schema_version = schema_version;
}

void free_page(FILE *file, Page *page) {
    DBHeader db_header;
    read_header(file, &db_header);

    page->is_leaf = UINT8_MAX;
    page->prev = UINT32_MAX;
    page->next = db_header.free_list;
    db_header.free_list = page->page_num;

    write_page(file, page->page_num, page);
    write_header(file, &db_header);
}

void init_db(FILE *file) {

    if (!file) {
        perror("Failed to create database file");
        exit(1);
    }

    DBHeader header = {
        .magic = MAGIC_NUMBER,
        .schema_version = 1,
        .page_size = PAGE_SIZE,
        .root_page = 0, // Initially, no root page
        .num_page = 1, // Initially, no root page
        .free_list = UINT32_MAX // Initially, no root page
    };

    Page root;
    init_page(&root, 0, 1, 1);

    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(DBHeader), 1, file);
    fflush(file);

    write_page(file, 0, &root);

    printf("Database initialized: %s\n", DB_FILENAME);
}

void traverse(FILE *file, uint32_t page_num, int depth) {
    Schema schema;
    Page page;
    read_page(file, page_num, &page);


    if(page.is_leaf) {
	Schema schema;
	get_schema(&schema);

	for (int i = 0; i < page.num_keys; i++) {
	    for(int k = 0; k < depth; k++) {
		printf("==========");
	    }
	    printf(" leaf Depth %d: Page %i| Parent Page = %d | index = %d | key = %d ", depth, page.page_num, page.parent_page_num, i, page.keys[i]);
	    print_row(page.values[i], &schema);
	}
    } else {
	traverse(file, page.children[0], depth + 1);
	for(int k = 0; k < depth; k++) {
	    printf("==========");
	}
	printf(" internal Depth %d: Page %i| Parent Page %d |numkeys %i| index = %d | key %i \n", depth, page.page_num, page.parent_page_num, page.num_keys, 0, page.keys[0]);
	for (int i = 1; i < page.num_keys; i++) {
	    traverse(file, page.children[i], depth + 1);
	    for(int k = 0; k < depth; k++) {
		printf("==========");
	    }
	    printf(" internal Depth %d: Page %i| Parent Page %d |index = %d | key %i \n", depth, page.page_num, page.parent_page_num, i, page.keys[i]);
	}
	traverse(file, page.children[page.num_keys], depth + 1);

    }
}

uint32_t allocate_new_page(FILE *file) {
    DBHeader header;

    // Read DB header
    fseek(file, 0, SEEK_SET);
    fread(&header, sizeof(DBHeader), 1, file);
    uint32_t new_page_num; // Get next available page

    if (header.free_list != UINT32_MAX) {
	// Reuse a page from the free list
	Page free_page;
	read_page(file, header.free_list, &free_page);
	// Move free_list head to the next free page
	header.free_list = free_page.next;
	new_page_num = free_page.page_num;
    } else {
	new_page_num = header.num_page; // Get next available page
	header.num_page++; // Increment total pages
    }
    // Write updated header back to disk
    fseek(file, 0, SEEK_SET);
    fwrite(&header, sizeof(DBHeader), 1, file);
    fflush(file);

    return new_page_num;
}

uint32_t create_new_page(FILE *file, uint8_t is_leaf) {
    uint32_t page_num = allocate_new_page(file); // Allocate page number

    DBHeader header;
    read_header(file, &header);

    Page page;
    init_page(&page, page_num, is_leaf, header.schema_version);  // Initialize page in memory
    write_page(file, page_num, &page);  // Write page to disk

    return page_num;
}


void insert_v2(FILE *file, void **values) {
    // seek the correct leaf first
    DBHeader header;
    read_header(file, &header);
    uint32_t page_num = header.root_page;
    /*traverse(file, page_num, 0);*/

    // make first index of values as key
    uint32_t key;
    memcpy(&key, values[0], sizeof(uint32_t));

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
    insert_into_leaf(file, page_num, key, values);
}

int destroy_v2(FILE *file, uint32_t page_num, uint32_t key) {
    // seek the correct leaf first
    Page page;
    read_page(file, page_num, &page);

    DBHeader head;
    read_header(file, &head);
    /*traverse(file, head.root_page, 0);*/

    if(!page.is_leaf) {
	// uint32_t child_page_num = // logic for finding child_index
        int i = binary_search_internal_node(&page, key);

        uint32_t child_page_num = page.children[i];  // Go down the correct child
	int new_key = destroy_v2(file, child_page_num, key);

	Page child_page;
	read_page(file, child_page_num, &child_page);

	// assign new_key to parent keys if index > 0

	if (new_key >= 0 && i > 0) {
	    page.keys[i - 1] = new_key;
	    write_page(file, page.page_num, &page);
	    new_key = -1;
	}

	if (child_page.num_keys < MINIMUM_KEYS) {
	    bool borrowed = false;
	    Page left_child, right_child, moved_grandchild;
	    if(i < page.num_keys) {
		read_page(file, page.children[i+1], &right_child);

		if (right_child.num_keys > MINIMUM_KEYS) {
		    if(!right_child.is_leaf) {
			/*printf("========1======\n");*/

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
			/*printf("========2======\n");*/
			// claim right values to borrower
			memcpy(child_page.values[child_page.num_keys], right_child.values[0], MAX_ROW_SIZE);
			// in this case right_child.keys[0] == node.keys[childIndex]
			child_page.keys[child_page.num_keys] = right_child.keys[0];
			// claim first key of right as the new separator
			page.keys[i] = right_child.keys[1];
			// shift right values
			int j = 1;
			while (j < right_child.num_keys) {
			    right_child.keys[j - 1] = right_child.keys[j];
			    memcpy(right_child.values[j-1], right_child.values[j], MAX_ROW_SIZE);
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
			/*printf("========3======\n");*/
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
			/*printf("========4======\n");*/
			while (j > 0) {
			    child_page.keys[j] = child_page.keys[j-1];
			    memcpy(child_page.values[j], child_page.values[j-1], MAX_ROW_SIZE);
			    j--;
			}
			child_page.keys[0] = left_child.keys[left_child.num_keys-1];
			memcpy(child_page.values[0], left_child.values[left_child.num_keys-1], MAX_ROW_SIZE);
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
			/*printf("========5======\n");*/
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
			/*printf("========6======\n");*/
		    for (int j = 0; j < right_child.num_keys; j++) {
			child_page.keys[child_page.num_keys + j] = right_child.keys[j];
			memcpy(child_page.values[child_page.num_keys + j], right_child.values[j], MAX_ROW_SIZE);
		    }

		    child_page.num_keys += right_child.num_keys;
		    child_page.next = right_child.next;
		    if (right_child.next != UINT32_MAX) { // update prev pointer from next right
			Page next_right;
			read_page(file, right_child.next, &next_right);
			next_right.prev = child_page_num;
			write_page(file, right_child.next, &next_right);
		    }
		}
		for(int k = i; k < page.num_keys - 1; k++) {
			page.keys[k] = page.keys[k+1];
			page.children[k+1] = page.children[k+2];
		}
		page.num_keys--;
		// right child is now free, need to add free page list
		free_page(file, &right_child);
		write_page(file, page_num, &page);
		write_page(file, child_page_num, &child_page);
		borrowed = true;
	    }
	    if(!borrowed && i > 0) { // merge with left
		read_page(file, page.children[i-1], &left_child);
		// debug
		read_header(file, &head);
		// debug

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
			write_page(file, child_page.children[child_page.num_keys], &moved_grandchild);
			left_child.num_keys += child_page.num_keys + 1;
		} else {
			for(int k = 0; k < child_page.num_keys; k++) {
				left_child.keys[left_child.num_keys + k] = child_page.keys[k];
				memcpy(left_child.values[left_child.num_keys + k], child_page.values[k], MAX_ROW_SIZE);
			}
			
			left_child.num_keys += child_page.num_keys;
			left_child.next = child_page.next;
			if (child_page.next != UINT32_MAX) { // update prev pointer for next of child
			    Page next_child;
			    read_page(file, child_page.next, &next_child);
			    next_child.prev = page.children[i-1]; // page.children[i - 1] == left_child.page_num
			    write_page(file, child_page.next, &next_child);
			}
		}

		for(int j = i; j < page.num_keys; j++) {
			page.keys[j-1] = page.keys[j];
			page.children[j] = page.children[j+1];
		}
		page.num_keys--;

		// child_page is now free, should add to freelist
		free_page(file, &child_page);
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
		free_page(file, &page);
		DBHeader db_header;
		read_header(file, &db_header);
		db_header.root_page = child_page.page_num;
		write_header(file, &db_header);
	    }
	}
	// code for splitting merging etc
	return new_key;
    } else {
	int j = binary_search_leaf(&page, key);

	if (page.keys[j] == key) {
	    int i = j;
	    while(i < page.num_keys-1) {
		page.keys[i] = page.keys[i+1];
		memcpy(page.values[i], page.values[i+1], MAX_ROW_SIZE);
		i++;
	    }
	    page.num_keys--;
	    write_page(file, page.page_num, &page);

	    if(j == 0) {
		// promote new key for its parent
		return page.keys[0];
	    }
	    return -1;
	} else {
	    printf("Key %d not found, mid: %i\n", key, j);
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
	parent.num_keys++;

	if(parent.num_keys >= MAXIMUM_KEYS) {    
	    Page second_half;
	    uint32_t second_half_page_num = create_new_page(file, 0);
	    read_page(file, second_half_page_num, &second_half);
	    
	    int new_promoted = parent.keys[MID];
	    
	    second_half.children[0] = parent.children[MID + 1];

	    // update child parent page num
	    Page child;
	    
	    for (int j = MID + 1; j < parent.num_keys; j++) {
		second_half.keys[j - (MID + 1)] = parent.keys[j];
		second_half.children[j - MID] = parent.children[j + 1];
		read_page(file, second_half.children[j - MID], &child);
		child.parent_page_num = second_half.page_num; 
		write_page(file, child.page_num, &child);
	    }

	    read_page(file, second_half.children[0], &child);
	    child.parent_page_num = second_half.page_num; 
	    write_page(file, child.page_num, &child);

	    if (i < MID) {
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

void insert_into_leaf(FILE *file, uint32_t page_num, uint32_t key, void **values) {
	// need to handle splitting if overflow
    DBHeader db_header;
    read_header(file, &db_header);
    Page page;
    read_page(file, page_num, &page);

    // check if key exist
    int left = binary_search_leaf(&page, key);

    int i = page.num_keys;

    if(left < page.num_keys && page.keys[left] == key) { // unique validation constraint
	printf("Key already exist! (key: %u)\n", key);
	return;
    }

    while (i > left && page.keys[i-1] > key) {
	page.keys[i] = page.keys[i-1];
	memcpy(page.values[i], page.values[i-1], MAX_ROW_SIZE);
	i--;
    }
    Schema schema;
    get_schema(&schema);
    
    page.keys[i] = key;

    for(int j = 0; j < schema.num_columns; j++) {
	Column col = schema.columns[j];
	void *src = values[j];
	memcpy(page.values[i] + col.offset, src, col.size);
    }
    page.num_keys++;
    // DEBUG
    write_page(file, page.page_num, &page);
    // DEBUG
    if(page.num_keys >= MAXIMUM_KEYS) {
	// DEBUG
	traverse(file, db_header.root_page, 0);
	traverse(file, page.page_num, 0);
	// DEBUG
	Page new_page;
	uint32_t new_page_num = create_new_page(file, 1);
	read_page(file, new_page_num, &new_page);
	int promoted_key = page.keys[MID];

	for(int k = MID; k < page.num_keys; k++) {
	    new_page.keys[k-MID] = page.keys[k];
	    memcpy(new_page.values[k-MID], page.values[k], MAX_ROW_SIZE);
	}

	new_page.num_keys = page.num_keys - MID;
	page.num_keys  = MID;

	new_page.parent_page_num = page.parent_page_num;
	new_page.prev = page_num;
	new_page.next = page.next;

	if(page.next != UINT32_MAX) {
	    Page next_page;
	    read_page(file, page.next, &next_page);
	    next_page.prev = new_page_num;
	    write_page(file, page.next, &next_page);
	}

	page.next = new_page_num;
	write_page(file, new_page_num, &new_page);
	write_page(file, page_num, &page);

	promote_to_parent_page(file, page.page_num, promoted_key, new_page_num);

    } else {
	write_page(file, page_num, &page);
    }
}


void range_key_query(FILE *file, uint32_t smallest, uint32_t biggest, int asc) {
    // if want to only look for value bigger than smallest, set biggest to UINT32_MAX
    // if want to only look for value smaller than biggest, set smallest to 0 (min uint32 val)

    DBHeader db_header;
    read_header(file, &db_header);
    uint32_t page_num = db_header.root_page;

    Page page; 
    read_page(file, page_num, &page);

    uint32_t start;
    Schema schema;

    get_schema(&schema);

    if (asc) {
	while(!page.is_leaf) {
	    page_num = page.children[binary_search_internal_node(&page, smallest)];
	    read_page(file, page_num, &page);
	}
	int left = binary_search_leaf(&page, smallest);
	printf("left: %d \n", left);

	for(int i = left; i < page.num_keys && page.keys[i] <= biggest; i++) {
	    print_row(page.values[i], &schema);
	}
	while(page.next != UINT32_MAX) {
	    read_page(file, page.next, &page);
	    for(int i = 0; i < page.num_keys && page.keys[i] <= biggest; i++) {
		print_row(page.values[i], &schema);
	    }
	}
    } else {
	while(!page.is_leaf) {
	    page_num = page.children[binary_search_internal_node(&page, biggest)];
	    read_page(file, page_num, &page);
	}
	int right = binary_search_leaf(&page, biggest);

	if (right >= page.num_keys) {
	    right = page.num_keys - 1;
	}
	printf("right: %d \n", right);

	for(int i = right; i >= 0 && page.keys[i] >= smallest; i--) {
	    print_row(page.values[i], &schema);
	}
	while(page.prev != UINT32_MAX) {
	    read_page(file, page.prev, &page);
	    for(int i = page.num_keys - 1; i >= 0 && page.keys[i] >= smallest; i--) {
		print_row(page.values[i], &schema);
	    }
	}
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
	    printf("Valid database file found. Schema Version: %i\n", db_header.schema_version);
	    printf("Valid database file found. Page Size: %i\n", db_header.page_size);
	    
	}

    }

    srand(time(NULL)); // random number seeding

    int population = 200;

    int used[population];
    memset(used, 0, sizeof(int) * population);
    int insert_ord[population];
    memset(insert_ord, 0, sizeof(int) * population);
    int arr[] = {
    98, 41, 51, 175, 153, 6, 61, 139, 176, 101, 135, 71, 59, 132, 172, 121, 194, 180, 168, 140,
    118, 192, 125, 42, 166, 25, 169, 87, 103, 195, 81, 65, 95, 155, 182, 22, 57, 129, 150, 111,
    79, 197, 144, 110, 77, 13, 40, 133, 2, 10, 157, 76, 107, 72, 126, 58, 3, 54, 82, 115, 39,
    53, 16, 156, 49, 64, 189, 86, 154, 108, 106, 97, 90, 123, 188, 66, 152, 181, 173, 161, 190,
    179, 23, 21, 122, 147, 178, 102, 35, 38, 33, 43, 94, 109, 145, 50, 75, 74, 17, 18, 78,
    120, 29, 174, 0, 52, 45, 186, 28, 141, 124, 48, 8, 116, 84, 149, 32, 34, 164, 63, 26,
    1, 170, 20, 158, 7, 96, 30, 146, 60, 191, 185, 44, 67, 198, 196, 112, 177, 151, 9, 46,
    62, 47, 114, 163, 165, 127, 130, 56, 68, 99, 105, 183, 159, 136, 31, 162, 119, 142, 143, 89, 131, 88, 15, 14, 37, 104, 193, 137, 148, 128, 117, 113, 91, 69, 134, 36, 85, 100, 184, 5,
    4, 93, 80, 199, 73, 167, 160, 24, 27, 55, 11, 171, 187, 92, 70, 19, 83
};
    population = sizeof(arr) / sizeof(arr[0]);
    /*for(int i = 0; i < population; i++) {*/
    for(int i = 0; i < population; i++) {
	void *values[2];
	int id ;
	do {
	    id = rand() % population;
	} while (used[id]);
	used[id] = 1;


	values[0] = &id;

	char username[10];
	snprintf(username, sizeof(username), "user%d", id);
	values[1] = username;
	insert_v2(file, values);
    }
    read_header(file, &db_header);
    printf("=======================TRAVSTARTS=================\n");
    traverse(file, db_header.root_page, 0);
    printf("=======================TRAVENDS=================\n");


    read_header(file, &db_header);
    memset(used, 0, sizeof(int) * population);
    traverse(file, db_header.root_page, 0);
    printf("=======================DESTROY=================\n");
	/*   for(int i = 2; i < population; i++) {*/
	/*printf("%d\n", insert_ord[i]);*/
	/*   }*/

   /*for(int i = 0; i < population - (population / 10); i++) {*/
    memset(used, 0, sizeof(int) * population);
   for(int i = 0; i < population; i++) {
	read_header(file, &db_header);
	int id ;
	do {
	    id = rand() % population;
	} while (used[id]);
	used[id] = 1;
	insert_ord[i] = id;

	printf("======== current id: %d\n", id);
	destroy_v2(file, db_header.root_page, id);
	read_header(file, &db_header);
	printf("=======================TRAVSTARTS AFTER DEST=================\n");
	traverse(file, db_header.root_page, 0);
	printf("=======================TRAVENDS AFTER DEST=================\n");
	printf("======== current id: %d\n", id);
   }
    read_header(file, &db_header);

    printf("=======================TRAVSTARTS AFTER DEST=================\n");
    traverse(file, db_header.root_page, 0);
    printf("=======================TRAVENDS AFTER DEST=================\n");
    printf("=======================reinsert from blank=================\n");

    for(int i = 0; i < 10; i++) {
	void *values[2];
	int id = i;


	values[0] = &id;

	char username[10];
	snprintf(username, sizeof(username), "user%d", id);
	values[1] = username;
	insert_v2(file, values);
    }
    printf("=======================TRAVSTARTS AFTER DEST=================\n");
    read_header(file, &db_header);
    traverse(file, db_header.root_page, 0);
    printf("=======================TRAVENDS AFTER DEST=================\n");


    /*traverse(file, db_header.root_page, 0);*/
    /*range_key_query(file, 0, 50, 1);*/
    /*range_key_query(file, 0, 50, 0);*/
    
    fclose(file);
    return 0;
}
