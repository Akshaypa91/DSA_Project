// init.c
#include "utils.h"

// ==== HashMap for tracking files ==== //
#define TABLE_SIZE 100

typedef struct FileNode {
	char filename[100];
	struct FileNode* next;
} FileNode;

FileNode* hashTable[TABLE_SIZE]; // global table

// Simple polynomial hash function
int hashFunc(char* str) {
	int hash = 0;
	for (int i = 0; str[i]; i++) {
		hash = (hash * 31 + str[i]) % TABLE_SIZE;
	}
	return hash;
}

// Insert file into tracking system
void trackFile(char* filename) {
	int idx = hashFunc(filename);
	FileNode* newNode = (FileNode*)malloc(sizeof(FileNode));
	strcpy(newNode->filename, filename);
	newNode->next = hashTable[idx];
	hashTable[idx] = newNode;
}

// Print tracked files (debug purpose)
void printTrackedFiles() {
	for (int i = 0; i < TABLE_SIZE; i++) {
		FileNode* temp = hashTable[i];
		while (temp) {
			printf("Tracked: %s\n", temp->filename);
			temp = temp->next;
		}
	}
}

// ==== Init repository ==== //
void initRepo() {
	// create .minigit folder
	mkdir(".minigit", 0700);

	// create empty commits file
	FILE* f = fopen(".minigit/commits.txt", "w");
	if (f) fclose(f);

	// initialize hash table
	for (int i = 0; i < TABLE_SIZE; i++) {
		hashTable[i] = NULL;
	}

	printf("Initialized empty MiniGit repository\n");
}