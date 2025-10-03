#include "utils.h"

void initRepo() {
	// Create .minigit folder
	makeDir(".minigit");

	// Create empty commits file
	FILE* f = fopen(".minigit/commits.txt", "w");
	if (f) fclose(f);

	// Initialize tracked files hash table
	initHashTable();

	printf("Initialized empty MiniGit repository\n");
}