// log.c
#include "utils.h"

void logCommits() {
    FILE* f = fopen(".minigit/commits.txt", "r");
    if (!f) {
        printf("No commits yet.\n");
        return;
    }

    char line[512];
    int hasCommits = 0;
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
            line[len-1] = '\0';
        if (strlen(line) > 0) {
            printf("%s\n", line);
            hasCommits = 1;
        }
    }
    fclose(f);

    if (!hasCommits) {
        printf("No commits yet.\n");
    }
}