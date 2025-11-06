#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//test2
void checkoutCommit(unsigned long commitId) {
    printf("(Commit checkout feature not implemented yet — safe placeholder)\n");
}

// Switch to a branch (simple HEAD update only)
void checkoutBranch(char* name) {
    FILE* f = fopen(".minigit/branches.txt", "r");
    if (!f) {
        printf("No branches exist yet.\n");
        return;
    }

    char bname[100];
    unsigned long cid;
    int found = 0;

    while (fscanf(f, "%s %lu", bname, &cid) == 2) {
        if (strcmp(bname, name) == 0) {
            found = 1;
            break;
        }
    }
    fclose(f);

    if (!found) {
        printf("Branch '%s' not found!\n", name);
        return;
    }

    FILE* headFile = fopen(".minigit/HEAD.txt", "w");
    if (!headFile) {
        printf("Error updating HEAD file.\n");
        return;
    }
    fprintf(headFile, "%s\n", name);
    fclose(headFile);

    printf("Switched to branch '%s'\n", name);
}