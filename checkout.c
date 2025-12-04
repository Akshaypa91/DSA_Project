#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#define DIR_EXISTS(path) (_access(path, 0) == 0)
#define COPY_CMD "xcopy /E /I /Y \"%s\" \".\" >nul"
#else
#include <unistd.h>
#include <sys/stat.h>
#define DIR_EXISTS(path) (!access(path, F_OK))
#define COPY_CMD "cp -r %s/* ."
#endif

// ---------------------------------------------------------------------
// 1) Checkout commit by string "c123456"
// ---------------------------------------------------------------------
void checkoutCommitByString(const char* commitID) {
    if (!commitID || strlen(commitID) == 0) {
        printf("Invalid commit ID!\n");
        return;
    }

    char folder[256];
    snprintf(folder, sizeof(folder), ".minigit/commits/%s", commitID);

    if (!DIR_EXISTS(folder)) {
        printf("Commit %s not found!\n", commitID);
        return;
    }

    char command[512];
    snprintf(command, sizeof(command), COPY_CMD, folder);
    system(command);

    printf("Checked out commit %s\n", commitID);
}

// ---------------------------------------------------------------------
// 2) Checkout commit by ONLY NUMBER (ex: 554060)
// ---------------------------------------------------------------------
void checkoutCommit(const char* commitID) {
    // commitID may be "554060", convert it to "c554060"
    if (!commitID) return;

    char buildID[64];

    // If user already typed cXXXXX, use as-is
    if (commitID[0] == 'c')
        strcpy(buildID, commitID);
    else
        snprintf(buildID, sizeof(buildID), "c%06lu", strtoul(commitID, NULL, 10));

    checkoutCommitByString(buildID);
}

// ---------------------------------------------------------------------
// 3) Checkout branch (NO changes, safe)
// ---------------------------------------------------------------------
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
