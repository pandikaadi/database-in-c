#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WAL_FILE "wal.log"
#define TABLE_SIZE 1024

typedef struct KVPair {
	char *key;
	char *value;
	struct KVPair *next;
} KVPair;

typedef struct {
	KVPair *buckets[TABLE_SIZE];
} KVStore;

KVStore store = {0}; // set binary to 0

// djb2hash
unsigned long hash(const char *key) {
	unsigned long hash = 5381;
	int c;

	while((c = *key++)) {
		hash = ((hash << 5) + hash ) + c;
	}
	return hash % TABLE_SIZE;
}


void kv_store_put(const char *key, const char *value) {
	unsigned long idx = hash(key);
	KVPair *entry = store.buckets[idx];

	while(entry) {
		if(strcmp(entry->key, key) == 0) {
			free(entry->value);
			entry->value = strdup(value);
			return;
		}
		entry = entry->next;
	}

	KVPair *new_pair = malloc(sizeof(KVPair));
	new_pair->key = strdup(key);
	new_pair->value = strdup(value);
	new_pair->next = store.buckets[idx];
	store.buckets[idx] = new_pair;
}

char *kv_store_get(const char *key) {
	unsigned long idx = hash(key);
	KVPair *entry = store.buckets[idx];

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
	kv_store_put(key,value);
}

void replay_wal() {
	FILE *file = fopen(WAL_FILE, "r");
	if (!file) {
		perror("Failed to open WAL file");
		exit(1);
	}
	char key[256], value[256];
	while (fscanf(file, "%255s %255s", key, value) == 2) {
		kv_store_put(key,value);
	}
	fclose(file);
}

void kv_store_free() {
	for(int i = 0; i < TABLE_SIZE; i++) {
		KVPair *entry = store.buckets[i];
		while(entry) {
			free(entry->key);
			free(entry->value);
			KVPair *tmp = entry;
			entry = entry->next;
			free(tmp);
		}
	}
}

int main() {
	replay_wal();
	log_write("user1", "dadang");
	log_write("user2", "maman");

	printf("user1 -> %s\n", kv_store_get("user1"));
	printf("user2 -> %s\n", kv_store_get("user2"));
	printf("user3 -> %s\n", kv_store_get("user3") ? kv_store_get("user3") : "not found");

	kv_store_free();  // Cleanup
}
