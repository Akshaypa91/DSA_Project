#ifndef COMMIT_H
#define COMMIT_H

#define MAX_HASH 64
#define MAX_CHILDREN 8

typedef struct CommitNode {
    char hash[MAX_HASH];
    char message[256];
    char timestamp[64];
    char parentHashes[MAX_CHILDREN][MAX_HASH];
    int parentCount;

    struct CommitNode* parents[MAX_CHILDREN]; // Pointers to parent nodes
    struct CommitNode* next;                  // Linked list of all commits
} CommitNode;

typedef struct {
    CommitNode* head; // All commits stored here
} CommitGraph;

// Function prototypes
CommitNode* createCommit(const char* hash, const char* msg, const char* time);
void addParent(CommitNode* child, CommitNode* parent);
void logCommits(CommitGraph* graph, CommitNode* branchHead);

#endif