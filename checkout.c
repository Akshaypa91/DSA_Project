// checkout.c
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ID_LEN  32
#define STACK_SIZE  100

// Stack for commit IDs
typedef struct {
    char ids[STACK_SIZE][MAX_ID_LEN];
    int top;
} CommitStack;

static CommitStack history; //for back navigation
static char currentCommit[MAX_ID_LEN] = "HEAD"; //default pointer

// Internal stack functions
static void initStack(CommitStack* s) {
    s->top = -1;
}
static int isEmpty(CommitStack* s) {
    return s->top == -1;
}
static int isFull(CommitStack* s) {
    return s->top == STACK_SIZE - 1;
}
static void push(CommitStack* s, const char* id) {
    if (isFull(s)) {
        printf("Commit history stack overflow!\n");
        return;
    }
    strcpy(s->ids[++s->top], id);
}
static char* pop(CommitStack* s) {
    if (isEmpty(s)) return NULL;
    return s->ids[s->top--];
}

// Internal helper:--> restore snapshot from commit folder
static void restoreSnapshot(const char* commitID) {
    char folder[256];
    snprintf(folder, sizeof(folder), ".minigit/commits/%s", commitID);

    if (!fileExists(folder)) {
        printf("Commit %s not found!\n", commitID);
        return;
    }

    //overwrite working directory files from commit folder
    char command[512];
    snprintf(command, sizeof(command), "cp -r %s/* . 2>/dev/null", folder);
    system(command);

    printf("Checked out commit %s\n", commitID);
}

// Public API functions

// Checkout commit by numeric ID (as declared in utils.h)
void checkoutCommit(unsigned long commitId) {
    char commitIDStr[MAX_ID_LEN];
    snprintf(commitIDStr, sizeof(commitIDStr), "%lu", commitId);

    if (strcmp(currentCommit, "HEAD") != 0) {
        push(&history, currentCommit);
    }
    strncpy(currentCommit, commitIDStr, MAX_ID_LEN);
    restoreSnapshot(commitIDStr);
}

// Optional:--> go back to previous commit
void checkoutBack() {
    char* prev = pop(&history);
    if (!prev) {
        printf("No previous commit to go back.\n");
        return;
    }
    strncpy(currentCommit, prev, MAX_ID_LEN);
    restoreSnapshot(prev);
}

// Show current commit
void showCurrentCommit() {
    printf("Currently at commit: %s\n", currentCommit);
}

// Initialize checkout system
void initCheckout() {
    initStack(&history);
}