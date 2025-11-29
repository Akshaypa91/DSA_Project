// init.c
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

void initRepo() {
	// Create .minigit folder (makeDir is void so just call it)
	makeDir(".minigit");
	makeDir(".minigit/commits");

	// Create empty commits file if it doesn't exist
	if (!fileExists(".minigit/commits.txt")) {
		FILE* f = fopen(".minigit/commits.txt", "w");
		if (!f) {
			fprintf(stderr, "Error: cannot create .minigit/commits.txt: %s\n", strerror(errno));
		} else {
			fclose(f);
		}
	}

	// Create an index/tracked-files file (optional)
	if (!fileExists(".minigit/index.txt")) {
		FILE* idx = fopen(".minigit/index.txt", "w");
		if (!idx) {
			fprintf(stderr, "Error: cannot create .minigit/index.txt: %s\n", strerror(errno));
		} else {
			fclose(idx);
		}
	}

	// Initialize hash table for tracked files
	initHashTable();

	// Reset commit head
	commitHead = NULL;

	printf("Initialized empty MiniGit repository in .minigit/\n");
}