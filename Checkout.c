#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HASH 64
#define MAX_CHILDREN 8

typedef struct CommitNode {
    char hash[MAX_HASH];
    struct CommitNode* parents[MAX_CHILDREN];
    int parentCount;
} CommitNode;

typedef struct StackNode {
    CommitNode* commit;
    struct StackNode* next;
} StackNode;

// Push to stack
void push(StackNode** top, CommitNode* commit) {
    StackNode* node = (StackNode*)malloc(sizeof(StackNode));
    node->commit = commit;
    node->next = *top;
    *top = node;
}

// Pop from stack
CommitNode* pop(StackNode** top) {
    if (!*top) return NULL;
    StackNode* temp = *top;
    CommitNode* commit = temp->commit;
    *top = temp->next;
    free(temp);
    return commit;
}

// DFS to find commit by hash
CommitNode* findCommitDFS(CommitNode* head, const char* targetHash) {
    StackNode* stack = NULL;
    push(&stack, head);

    while (stack) {
        CommitNode* current = pop(&stack);
        if (strcmp(current->hash, targetHash) == 0) {
            return current; // found
        }
        for (int i = 0; i < current->parentCount; i++) {
            push(&stack, current->parents[i]);
        }
    }
    return NULL; // not found
}

// Checkout function
void checkout(CommitNode* branchHead, const char* target) {
    // Try to find commit in graph
    CommitNode* targetCommit = findCommitDFS(branchHead, target);

    if (targetCommit) {
        printf("Checked out commit: %s\n", targetCommit->hash);
    } else {
        // Could be a branch name, check branch head (pseudo)
        printf("Branch or commit not found: %s\n", target);
    }
}