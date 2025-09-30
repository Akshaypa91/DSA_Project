#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MINI_GIT_DIR ".mgit"
#define OBJECTS_DIR MINI_GIT_DIR "/objects"
#define COMMITS_DIR MINI_GIT_DIR "/commits"
#define HEAD_FILE MINI_GIT_DIR "/HEAD.txt"
#define INDEX_FILE MINI_GIT_DIR "/index.txt"
#define BRANCHES_FILE MINI_GIT_DIR "/branches.txt"

// Simple custom hash (like in your C++ version, not cryptographic)
char* Hash(const char* content) {
	static char hashStr[64];
	unsigned long hash = 5381;
	int c;
	while ((c = *content++)) {
		hash = ((hash << 5) + hash) + c; // djb2 hash
	}
	snprintf(hashStr, sizeof(hashStr), "%lu", hash);
	return hashStr;
}

// Get current time as string
char* getCurrentTime() {
	time_t now = time(NULL);
	char* dt = ctime(&now);
	return dt; // already null-terminated, includes newline
}

// Check if directory exists
int dir_exists(const char* path) {
	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Create directory
void create_dir(const char* path) {
	if (mkdir(path, 0777) != 0) {
		perror("mkdir failed");
	}
}

void init() {
	if (dir_exists(MINI_GIT_DIR)) {
		printf("mgit is already initialized.\n");
		return;
	}

	create_dir(MINI_GIT_DIR);
	create_dir(OBJECTS_DIR);
	create_dir(COMMITS_DIR);

	// HEAD file
	FILE* headFile = fopen(HEAD_FILE, "w");
	if (headFile) {
		fprintf(headFile, "main");
		fclose(headFile);
	}

	// branches.txt
	FILE* branchFile = fopen(BRANCHES_FILE, "w");
	if (branchFile) {
		fprintf(branchFile, "main:null\n");
		fclose(branchFile);
	}

	// index.txt (empty staging area)
	FILE* indexFile = fopen(INDEX_FILE, "w");
	if (indexFile) fclose(indexFile);

	printf("Initialized empty mgit repository.\n");
}