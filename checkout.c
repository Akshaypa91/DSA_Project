// checkout.c
#include "utils.h"

#define MAX_ID_LEN  32
#define STACK_SIZE  100

// ==== Stack for commit IDs ==== //
typedef struct {
	char ids[STACK_SIZE][MAX_ID_LEN];
	int top;
} CommitStack;

static CommitStack history;       // for back navigation
static char currentCommit[MAX_ID_LEN] = "HEAD"; // default pointer

// ==== Stack Functions ==== //
void initStack(CommitStack* s) {
	s->top = -1;
}

int isEmpty(CommitStack* s) {
	return s->top == -1;
}

int isFull(CommitStack* s) {
	return s->top == STACK_SIZE - 1;
}

void push(CommitStack* s, const char* id) {
	if (isFull(s)) {
		printf("Commit history stack overflow!\n");
		return;
	}
	strcpy(s->ids[++s->top], id);
}

char* pop(CommitStack* s) {
	if (isEmpty(s)) return NULL;
	return s->ids[s->top--];
}

char* peek(CommitStack* s) {
	if (isEmpty(s)) return NULL;
	return s->ids[s->top];
}

// ==== Restore Snapshot ==== //
static void restoreSnapshot(const char* commitID) {
	char folder[256];
	snprintf(folder, sizeof(folder), ".minigit/commits/%s", commitID);

	if (!fileExists(folder)) {
		printf("Commit %s not found!\n", commitID);
		return;
	}

	// read commit folder files
	// Note: simple version = restore files listed in .minigit/commits.txt
	FILE* f = fopen(".minigit/commits.txt", "r");
	if (!f) {
		printf("No commit history!\n");
		return;
	}

	char line[512];
	int found = 0;
	while (fgets(line, sizeof(line), f)) {
		char id[MAX_ID_LEN], msg[256];
		if (sscanf(line, "%s : %[^\n]", id, msg) == 2) {
			if (strcmp(id, commitID) == 0) {
				found = 1;
				break;
			}
		}
	}
	fclose(f);

	if (!found) {
		printf("Commit %s not found in log!\n", commitID);
		return;
	}

	// For simplicity: overwrite working directory files
	// (Assume file list matches staging)
	char command[512];
	snprintf(command, sizeof(command), "cp -r %s/* .", folder);
	system(command);

	printf("Checked out commit %s\n", commitID);
}

// ==== Checkout to specific commit ==== //
void checkout(const char* commitID) {
	if (strcmp(currentCommit, "HEAD") != 0) {
		// push current commit to history stack
		push(&history, currentCommit);
	}
	strncpy(currentCommit, commitID, MAX_ID_LEN);

	restoreSnapshot(commitID);
}

// ==== Go back to previous commit ==== //
void checkoutBack() {
	char* prev = pop(&history);
	if (!prev) {
		printf("No previous commit to go back.\n");
		return;
	}
	strncpy(currentCommit, prev, MAX_ID_LEN);
	restoreSnapshot(prev);
}

// ==== Show current commit ==== //
void showCurrentCommit() {
	printf("Currently at commit: %s\n", currentCommit);
}

// ==== Initialize checkout system ==== //
void initCheckout() {
	initStack(&history);
}