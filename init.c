#include "utils.h"

void initRepo() {
    // Create .minigit folder
    makeDir(".minigit");

    // Create empty commits file
    FILE* f = fopen(".minigit/commits.txt", "w");
    if (f) fclose(f);

    // Initialize hash table for tracked files
    initHashTable(); //Add this line

    // Reset commit head
    commitHead = NULL;

    printf("Initialized empty MiniGit repository\n");
}