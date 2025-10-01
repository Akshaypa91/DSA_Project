#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME 128
#define MAX_HASH 64

// Linked list node for file-hash mapping
typedef struct FileEntry {
    char filename[MAX_FILENAME];
    char hash[MAX_HASH];
    struct FileEntry* next;
} FileEntry;

// Read commit file into linked list of FileEntry
FileEntry* readCommitFiles(const char* commitHash) {
    char path[256];
    snprintf(path, sizeof(path), "commits/%s", commitHash);

    FILE* file = fopen(path, "r");
    if (!file) {
        return NULL;
    }

    FileEntry* head = NULL;
    FileEntry* tail = NULL;
    char line[256];

    while (fgets(line, sizeof(line), file)) {
        // Skip metadata lines
        if (strncmp(line, "message:", 8) == 0 ||
            strncmp(line, "timestamp:", 10) == 0 ||
            strncmp(line, "parent:", 7) == 0 ||
            strncmp(line, "branch:", 7) == 0) {
            continue;
        }

        char* sep = strchr(line, ':');
        if (!sep) continue;

        *sep = '\0';
        char* filename = line;
        char* hash = sep + 1;
        hash[strcspn(hash, "\n")] = '\0'; // remove newline

        FileEntry* entry = (FileEntry*)malloc(sizeof(FileEntry));
        strncpy(entry->filename, filename, MAX_FILENAME);
        strncpy(entry->hash, hash, MAX_HASH);
        entry->next = NULL;

        if (!head) head = entry;
        else tail->next = entry;
        tail = entry;
    }

    fclose(file);
    return head;
}

// Compare two commits (diff)
void diff(const char* commit1, const char* commit2) {
    FileEntry* list1 = readCommitFiles(commit1);
    FileEntry* list2 = readCommitFiles(commit2);

    if (!list1 || !list2) {
        printf("One of the commits or both are not found.\n");
        return;
    }

    printf("### Diff between %s and %s ###\n", commit1, commit2);

    // Check for removed/modified files
    for (FileEntry* e1 = list1; e1; e1 = e1->next) {
        FileEntry* e2 = list2;
        int found = 0;
        while (e2) {
            if (strcmp(e1->filename, e2->filename) == 0) {
                found = 1;
                if (strcmp(e1->hash, e2->hash) != 0) {
                    printf("%s was modified.\n", e1->filename);
                }
                break;
            }
            e2 = e2->next;
        }
        if (!found) {
            printf("%s was removed in %s\n", e1->filename, commit2);
        }
    }

    // Check for added files
    for (FileEntry* e2 = list2; e2; e2 = e2->next) {
        FileEntry* e1 = list1;
        int found = 0;
        while (e1) {
            if (strcmp(e1->filename, e2->filename) == 0) {
                found = 1;
                break;
            }
            e1 = e1->next;
        }
        if (!found) {
            printf("%s was added in %s\n", e2->filename, commit2);
        }
    }
}

// Free linked list memory
void freeFileList(FileEntry* head) {
    while (head) {
        FileEntry* tmp = head;
        head = head->next;
        free(tmp);
    }
}

// Example main for testing
int main() {
    diff("commitHash1", "commitHash2");
    return 0;
}