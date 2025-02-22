#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TABLE_SIZE 100

typedef struct KeyValue {
	char *Key;
	char *Value;
	struct KeyValue *next;
} KeyValue;

typedef struct {
	KeyValue **buckets;
	unsigned int size;
	unsigned int count;	
} HashTable;

void put(HashTable **table_ptr, const char *key, const char *value);

unsigned int hash(const char *key, unsigned int size) {
	unsigned int hash = 0;
	while(*key) {
		hash = (hash * 31) + *key;
		key++;
	}
	return hash % size;
}

HashTable *create_table(unsigned int table_size) {
	HashTable *table = malloc(sizeof(HashTable));
	if (!table) {
		exit(1);
	}
	table->size = table_size;
	table->count = 0;
	table->buckets = calloc(table->size, sizeof(KeyValue *));

	return table;
}


void free_table(HashTable *table) {
	for(int i = 0; i < table->size; i++) {
		KeyValue *pair = table->buckets[i];
		while(pair) {
     			KeyValue *temp = pair;
			pair = pair->next;
			free(temp->Key);
			free(temp->Value);
			free(temp);
     		}
	}
	free(table-> buckets);
	free(table);
}

void resize_table(HashTable **old_table_ptr, int new_size) {
	HashTable *old_table = *old_table_ptr;
	HashTable *new_table = create_table(new_size);

	for(int i = 0; i < old_table->size; i++) {
		KeyValue *current = old_table->buckets[i];
		while (current) {
			put(&new_table, current->Key, current->Value);
			current = current->next;
		}
	}
	HashTable *temp = *old_table_ptr;
	*old_table_ptr = new_table;
	free_table(temp);

}

void put(HashTable **table_ptr, const char *key, const char *value) {
	HashTable *table = *table_ptr;
	unsigned int index = hash(key, table->size);
	KeyValue *current = table->buckets[index];
	KeyValue *prev = NULL;

	while(current) {
		if (strcmp(key, current->Key) == 0) {
			free(current->Value);
			current->Value = strdup(value);
			if(!current->Value) {
				exit(1);
			}
			return;
		}
		prev = current;
		current = current->next;
	}

	KeyValue *new_pair = malloc(sizeof(KeyValue));
	if (!new_pair) {
		return ;
	}

	new_pair->Key = strdup(key);
	new_pair->Value = strdup(value);
	if (!new_pair->Key || !new_pair->Value) {  // Check strdup failure
		free(new_pair->Key);
		free(new_pair->Value);
		free(new_pair);
		exit(1)
	}
	new_pair->next = NULL;

	if (!prev) {
		table->buckets[index] = new_pair;
	} else {
		/*putting at the end of linked_list*/
		prev->next = new_pair;
	}

	table->count++;

	printf("table size: %i \n", table->size);

	if (table->count > table->size * 2 / 3) {
		resize_table(table_ptr, table->size * 2);
	}
}
/**/
/**/
char *get(HashTable *table, const char *key) {
	unsigned int index = hash(key, table->size);
	KeyValue *pair = table->buckets[index];

	while(pair) {
		if (strcmp(pair->Key, key) == 0) {
			return pair->Value;
		}
		pair = pair->next;
	}
	return NULL;
}

void remove_key(HashTable **table_ptr, const char *key) {
	HashTable *table = *table_ptr;
	unsigned int index = hash(key, table->size);
	KeyValue *pair = table->buckets[index];
	KeyValue *prev = NULL;

	while(pair) {

		if(strcmp(key, pair->Key) == 0) {
			if(prev) {
				prev->next = pair->next;
			} else {
				table->buckets[index] = pair->next;
			}
			table->count -= 1;
			free(pair->Key);
			free(pair->Value);
			free(pair);
			

			unsigned int current_count = table->count;

			if ( (float)current_count / table->size < 0.2 ) {
				int new_size = table->size / 2;
				if (new_size > 1) {
					resize_table(table_ptr, new_size );
				}

			}
			return;
		}
		prev = pair;
		pair = pair->next;
	}

}


int main() {
	HashTable *table = create_table(1);  // Start with small size for testing

	    printf("\n--- Testing Hash Collisions ---\n");

	put(&table, "a", "First");
	put(&table, "b", "Second");
	put(&table, "c", "Third");

	unsigned int index_a = hash("a", table->size);
	unsigned int index_b = hash("b", table->size);
	unsigned int index_c = hash("c", table->size);

	printf("Index of 'a': %u\n", index_a);
	printf("Index of 'b': %u\n", index_b);
	printf("Index of 'c': %u\n", index_c);

	printf("\nRetrieving values after collision insertions:\n");
	printf("'a' -> %s\n", get(table, "a"));
	printf("'b' -> %s\n", get(table, "b"));
	printf("'c' -> %s\n", get(table, "c"));

	printf("\nInspecting linked list at bucket[%u]:\n", index_a);
	KeyValue *current = table->buckets[index_a];
	while (current) {
	    printf("  Key: %s, Value: %s\n", current->Key, current->Value);
	    current = current->next;
	}

	printf("'a' -> %s\n", get(table, "a"));
	printf("'b' -> %s\n", get(table, "b"));
	printf("'c' -> %s\n", get(table, "c"));

	printf("size: %i %i \n", table->size, table->count);
	remove_key(&table, "a");
	printf("size: %i %i \n", table->size, table->count);
	remove_key(&table, "b");
	printf("size: %i %i \n", table->size, table->count);
	remove_key(&table, "c");
	printf("size: %i %i\n", table->size, table->count);

    free_table(table);
}
