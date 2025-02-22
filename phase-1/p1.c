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
	KeyValue *buckets[TABLE_SIZE];
} HashTable;

unsigned int hash(const char *key) {
	unsigned int hash = 0;
	while(*key) {
		hash = (hash * 31) + *key;
		key++;
	}
	return hash % TABLE_SIZE;
}

HashTable *create_table() {
	HashTable *table = malloc(sizeof(HashTable));
	if (!table) {
		exit(1);
	}
	memset(table->buckets, 0, sizeof(table->buckets));
	return table;
}

void put(HashTable *table, const char *key, const char *value) {
	unsigned int index = hash(key);
	KeyValue *new_pair = malloc(sizeof(KeyValue));
	
	if (!new_pair) {
		return;
	}

	new_pair->Key = strdup(key);
	new_pair->Value = strdup(value);
	new_pair->next = table->buckets[index];
	table->buckets[index] = new_pair;
}


char *get(HashTable *table, const char *key) {
	unsigned int index = hash(key);
	KeyValue *pair = table->buckets[index];

	while(pair) {
		if (strcmp(pair->Key, key) == 0) {
			return pair->Value;
		}
		pair = pair->next;
	}
	return NULL;
}

void free_table(HashTable *table) {
	for(int i = 0; i < TABLE_SIZE; i++) {
		KeyValue *pair = table->buckets[i];
		while(pair) {
     			KeyValue *temp = pair;
			pair = pair->next;
			free(temp->Key);
			free(temp->Value);
			free(temp);
     		}
	}
	free(table);
}


int main() {
	HashTable *table = create_table();

	put(table, "name", "Dadang");
	put(table, "age", "25");
	put(table, "city", "Lombok");

	printf("Name: %s\n", get(table, "name"));
	printf("Age: %s\n", get(table, "age"));
	printf("City: %s\n", get(table, "city"));

	free_table(table);
}
