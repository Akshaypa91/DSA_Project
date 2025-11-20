#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

// Forward declarations
void enqueuePush(const char* commitID);
void pullCommit(const char* commitID);

// Push to remote repository
void pushToRemote(char* remotePath) {
	if (!remotePath) return;
	enqueuePush(remotePath);
}

// Pull from remote repository
void pullFromRemote(char* remotePath) {
	if (!remotePath) return;
	pullCommit(remotePath);
}

void pushToRemote(char* remotePath) {
    if (!remotePath) {
        printf("Invalid remote path\n");
        return;
    }

    char command[256];
    snprintf(command, sizeof(command),
             "cp -r .mini_git %s/", remotePath);

    int result = system(command);

    if (result == 0)
        printf("Push successful! Copied repository to %s\n", remotePath);
    else
        printf("Push failed.\n");
}


void pullCommit(const char* commitID) {
	printf("Pulling commit %s from remote...\n", commitID);

	// Simulate pull logic
	char command[256];
	snprintf(command, sizeof(command), "cp -r %s/.minigit ./", commitID);
	int result = system(command);

	if (result == 0)
		printf("Pull successful! Commit %s restored from remote.\n", commitID);
	else
		printf("Pull failed. Commit not found in remote.\n");
}
