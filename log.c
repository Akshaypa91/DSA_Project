#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commit.h"

// Recursive DFS log traversal
void printLogDFS(CommitNode* commit) {
    if (!commit) return;

    printf("Commit: %s\n", commit->hash);
    printf("message: %s\n", commit->message);
    printf("timestamp: %s\n", commit->timestamp);

    for (int i = 0; i < commit->parentCount; i++) {
        printf("parent: %s\n", commit->parentHashes[i]);
    }
    printf("###################################\n");

    // Traverse parents recursively
    for (int i = 0; i < commit->parentCount; i++) {
        printLogDFS(commit->parents[i]);
    }
}

// Public log function – starts from branch head
void logCommits(CommitGraph* graph, CommitNode* branchHead) {
    if (!branchHead) {
        printf("No commits in this branch.\n");
        return;
    }
    printf("=== Commit Log ===\n");
    printLogDFS(branchHead);
}