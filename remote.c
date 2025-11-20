#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

// Push local repo to remote
void pushToRemote(char* remotePath) {
    if (!remotePath) {
        printf("Invalid remote path\n");
        return;
    }

    printf("Pushing repository to remote: %s\n", remotePath);

    char command[512];

    // Windows xcopy command
    snprintf(command, sizeof(command),
             "xcopy .mini_git %s\\.mini_git /E /I /Y >nul", remotePath);

    int result = system(command);

    if (result == 0)
        printf("Push successful! Copied repository to %s\n", remotePath);
    else
        printf("Push failed.\n");
}

// Pull remote repo to local folder
void pullFromRemote(char* remotePath) {
    if (!remotePath) {
        printf("Invalid remote path\n");
        return;
    }

    printf("Pulling repository from remote: %s\n", remotePath);

    char command[512];

    // Windows xcopy command
    snprintf(command, sizeof(command),
             "xcopy %s\\.mini_git .\\.mini_git /E /I /Y >nul", remotePath);

    int result = system(command);

    if (result == 0)
        printf("Pull successful! Repository restored from %s\n", remotePath);
    else
        printf("Pull failed. Remote repository not found.\n");
}

