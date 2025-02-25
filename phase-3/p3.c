#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WAL_FILE "wal.log"
#define DB_FILE "db.dat"
#define TABLE_SIZE 1024

typedef struct Node {
	char key[256];
	char value[256];
	struct Node *next;
} Node;

Node *hash_table[TABLE_SIZE];
// djb2hash
unsigned long hash(const char *key) {
	unsigned long hash = 5381;
	int c;

	while((c = *key++)) {
		hash = ((hash << 5) + hash ) + c;
	}
	return hash % TABLE_SIZE;
}


void insert(const char *key, const char *value) {
	unsigned long idx = hash(key);


	Node *new_pair = malloc(sizeof(Node));

	if (!new_pair) {
		exit(1);
	}

	strcpy(new_pair->key, key);
	strcpy(new_pair->value, value);
	new_pair->next = hash_table[idx];

	hash_table[idx] = new_pair;

}

char *get(const char *key) {
	unsigned long idx = hash(key);
	Node *entry = hash_table[idx];

	while(entry) {
		if(strcmp(entry->key, key) == 0) 
			return entry->value;
		entry = entry->next;
	}
	return NULL;
}

void log_write(const char *key, const char *value) {
	FILE *file = fopen(WAL_FILE, "a");
	if (!file) {
		perror("Failed to open WAL file");
		exit(1);
	}
	fprintf(file, "%s %s\n", key, value);
	fflush(file); // Ensure data is written to disk
	fclose(file);
	insert(key,value);
}

void replay_wal() {
	FILE *file = fopen(WAL_FILE, "r");
	if (!file) {
		perror("Failed to open WAL file");
		exit(1);
	}
	char key[256], value[256];
	while (fscanf(file, "%255s %255s", key, value) == 2) {
		insert(key,value);
	}
	fclose(file);
}

void node_free() {
	for(int i = 0; i < TABLE_SIZE; i++) {
		Node *entry = hash_table[i];
		while(entry) {
			Node *tmp = entry;
			entry = entry->next;
			free(tmp);
		}
	}
}

void persist_to_file() {

	FILE *file = fopen(DB_FILE, "wb");
	if (!file) {
		perror("Failed to open DB file");
		exit(1);
	}

	for(int i = 0; i < TABLE_SIZE; i++) {
		Node *entry = hash_table[i];
		while(entry) {
			size_t key_len = strlen(entry->key)+1;
			size_t val_len = strlen(entry->value)+1;
			fwrite(&key_len, sizeof(size_t), 1, file);
			fwrite(&val_len, sizeof(size_t), 1, file);
			fwrite(entry->key, sizeof(char), key_len, file);
			fwrite(entry->value, sizeof(char), val_len, file);
			entry = entry->next;
		}
	}

}

void read_from_file() {
	FILE *file = fopen(DB_FILE, "rb");
	if (!file) {
		return;
	}
	while(!feof(file)) {
		size_t key_len, val_len;
		if(fread(&key_len, sizeof(size_t), 1, file) != 1) break;
		if(fread(&val_len, sizeof(size_t), 1, file) != 1) break;

		char *key = malloc(key_len);
		char *value = malloc(val_len);

		if(!key || !value) {
			perror("Failed to allocate memory");
			exit(1);
		}

		fread(key, sizeof(char), key_len, file);
		fread(value, sizeof(char), val_len, file);

		insert(key, value);
		free(key);
		free(value);
	}

	

}

void print_table() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        Node *entry = hash_table[i];
        while (entry) {
            printf("%s -> %s\n", entry->key, entry->value);
            entry = entry->next;
        }
    }
}

int main() {
	read_from_file();  // Load hash table from file on startup

	insert("user1", "dadang");
	insert("user2", "maman");
	insert("user3", "nasrul");

	printf("Hash table after insertions:\n");
	print_table();

	persist_to_file();  // Persist the hash table to a file

	node_free();  // Cleanup
}
