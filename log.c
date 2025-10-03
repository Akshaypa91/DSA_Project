#include "utils.h"

void logCommits() {
    FILE* f = fopen(".minigit/commits.txt", "r");
    if (!f) {
        printf("No commits yet.\n");
        return;
    }

    char line[512];
    int found = 0;
    while (fgets(line, sizeof(line), f)) {
        char id[32], msg[256];
        if (sscanf(line, "%s : %[^\n]", id, msg) == 2) {
            found = 1;
            printf("Commit ID: %s\n", id);
            printf("Message: %s\n", msg);
            // optionally read timestamp if stored
            printf("-----------------------------\n");
        }
    }
    if (!found) {
        printf("No commits yet.\n");
    }
    fclose(f);
}