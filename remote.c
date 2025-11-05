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

// Internal helpers
void enqueuePush(const char* commitID) {
	printf("Pushing commit %s to remote...\n", commitID);

	// Simulate push logic
	char command[256];
	snprintf(command, sizeof(command), "cp -r .minigit %s/", commitID);
	int result = system(command);

	if (result == 0)
		printf("Push successful! Commit %s copied to remote.\n", commitID);
	else
		printf("Push failed. Please check the remote path.\n");
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