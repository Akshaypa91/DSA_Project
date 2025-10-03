// commit.c
#include "utils.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MSG_LEN 256
#define MAX_ID_LEN 32

static int commitCount = 0; // local counter for IDs only

// ==== Generate Commit ID ==== //
static char* generateCommitID(const char* message) {
    static char id[MAX_ID_LEN];
    unsigned long h = simpleHash((char*)message) ^ ((commitCount + 1) * 131);
    snprintf(id, MAX_ID_LEN, "c%lu", h % 1000000);
    return id;
}

// ==== Save snapshot of staged files ==== //
static void saveSnapshot(const char* commitID) {
    char folder[256];
    snprintf(folder, sizeof(folder), ".minigit/commits/%s", commitID);
    makeDir(".minigit/commits");
    makeDir(folder);

    FILE* f = fopen(".minigit/staging.txt", "r");
    if (!f) {
        printf("Nothing to commit! (staging empty)\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
            if (len > 1 && line[len - 2] == '\r') line[len - 2] = '\0';
        }

        char* content = readFile(line);
        if (!content) {
            printf("Warning: could not read %s\n", line);
            continue;
        }

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", folder, line);
        writeFile(path, content);
        free(content);
    }

    fclose(f);
}

// ==== Make a new commit ==== //
void commitChanges(char* msg) {
    FILE* f = fopen(".minigit/staging.txt", "r");
    if (!f) {
        printf("Staging area is empty. Nothing to commit.\n");
        return;
    }
    fclose(f);

    char* id = generateCommitID(msg);

    // Create commit node (linked list in memory)
    Commit* newCommit = (Commit*)malloc(sizeof(Commit));
    newCommit->commitId = simpleHash(id);
    strncpy(newCommit->message, msg, sizeof(newCommit->message));
    newCommit->timestamp = time(NULL);
    newCommit->parent = commitHead;
    commitHead = newCommit;

    // Save staged files
    saveSnapshot(id);

    // Append commit info to commits.txt
    FILE* logFile = fopen(".minigit/commits.txt", "a");
    if (logFile) {
        fprintf(logFile, "%s : %s\n", id, msg);
        fclose(logFile);
    }

    // Clear staging area
    f = fopen(".minigit/staging.txt", "w");
    if (f) fclose(f);

    commitCount++;
    printf("Committed as %s : %s\n", id, msg);
}