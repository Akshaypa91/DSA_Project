#include "utils.h"

// ======= GLOBAL VARIABLES ======= //
FileNode* stagingHead = NULL;
FileNode* stagingTail = NULL;

Commit* commitHead = NULL;
Branch* branchHead = NULL;

// ======= File Tracking Hash Table ======= //
#define TABLE_SIZE 100
FileNode* hashTable[TABLE_SIZE];  // hash table for tracked files

// Simple hash function
int hashFunc(const char* str) {
	int hash = 0;
	for (int i = 0; str[i]; i++) {
		hash = (hash * 31 + str[i]) % TABLE_SIZE;
	}
	return hash;
}

// Track file in hash table
void trackFile(const char* filename) {
	int idx = hashFunc(filename);
	FileNode* newNode = (FileNode*)malloc(sizeof(FileNode));
	strcpy(newNode->filename, filename);
	newNode->next = hashTable[idx];
	hashTable[idx] = newNode;
}

// Debug: print tracked files
void printTrackedFiles() {
	for (int i = 0; i < TABLE_SIZE; i++) {
		FileNode* temp = hashTable[i];
		while (temp) {
			printf("Tracked: %s\n", temp->filename);
			temp = temp->next;
		}
	}
}

// Initialize hash table (call this in initRepo)
void initHashTable() {
	for (int i = 0; i < TABLE_SIZE; i++)
		hashTable[i] = NULL;
}

// ======= UTILITY HELPERS ======= //
unsigned long simpleHash(char* str) { return 0; } // implement as needed

void makeDir(const char* folder) {
#ifdef _WIN32
	_mkdir(folder);
#else
	mkdir(folder, 0700);
#endif
}

char* readFile(const char* filename) {
	FILE* f = fopen(filename, "r");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* buffer = (char*)malloc(size + 1);
	if (!buffer) { fclose(f); return NULL; }

	fread(buffer, 1, size, f);
	buffer[size] = '\0';
	fclose(f);
	return buffer;
}

void writeFile(const char* filename, const char* content) {
	FILE* f = fopen(filename, "w");
	if (!f) {
		printf("Error: Cannot write to file %s\n", filename);
		return;
	}
	fprintf(f, "%s", content);
	fclose(f);
}

void printSeparator() { printf("===========================\n"); }
int fileExists(const char* filename) { 
	FILE* f = fopen(filename, "r");
	if (!f) return 0;
	fclose(f);
	return 1;
}