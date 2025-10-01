#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRANCHES_FILE "branches.txt"
#define MAX_BRANCH 64
#define MAX_HASH 64

// Helper to get current branch from HEAD.txt
void getCurrentBranch(char* branch) {
    FILE* head = fopen("HEAD.txt", "r");
    if (!head) {
        strcpy(branch, "main"); // default
        return;
    }
    fscanf(head, "%s", branch);
    fclose(head);
}

// Helper to get branch head (latest commit hash)
void getBranchHead(const char* branchName, char* headHash) {
    FILE* f = fopen(BRANCHES_FILE, "r");
    if (!f) {
        strcpy(headHash, "null");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char* sep = strchr(line, ':');
        if (!sep) continue;
        *sep = '\0';
        char* name = line;
        char* hash = sep + 1;
        hash[strcspn(hash, "\n")] = '\0'; // remove newline
        if (strcmp(name, branchName) == 0) {
            strcpy(headHash, hash);
            fclose(f);
            return;
        }
    }

    strcpy(headHash, "null");
    fclose(f);
}

// Create a new branch
void createBranch(const char* branchName) {
    char currentBranch[MAX_BRANCH];
    getCurrentBranch(currentBranch);

    char currentHead[MAX_HASH];
    getBranchHead(currentBranch, currentHead);

    FILE* f = fopen(BRANCHES_FILE, "a");
    if (!f) {
        printf("Error opening branches file.\n");
        return;
    }

    fprintf(f, "%s:%s\n", branchName, currentHead);
    fclose(f);

    printf("Created branch: %s\n", branchName);
}