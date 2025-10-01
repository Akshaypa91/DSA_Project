#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FileNode {
    char filename[128];
    char hash[64];
    struct FileNode* next;
} FileNode;

typedef struct {
    FileNode* head;
    FileNode* tail;
} FileQueue;

void initQueue(FileQueue* q) { q->head = q->tail = NULL; }

void enqueue(FileQueue* q, const char* filename, const char* hash) {
    FileNode* node = (FileNode*)malloc(sizeof(FileNode));
    strcpy(node->filename, filename);
    strcpy(node->hash, hash);
    node->next = NULL;
    if (!q->head) q->head = q->tail = node;
    else { q->tail->next = node; q->tail = node; }
}

// Check if file exists in queue
FileNode* findFile(FileQueue* q, const char* filename) {
    FileNode* cur = q->head;
    while(cur) {
        if(strcmp(cur->filename, filename) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

// Merge function
void merge(const char* currentCommit, const char* targetCommit) {
    FileQueue merged;
    initQueue(&merged);

    char path[256];
    char line[256];
    char filename[128], hash[64];

    // Load target commit files
    snprintf(path, sizeof(path), "%s/%s", COMMITS_DIR, targetCommit);
    FILE* f = fopen(path, "r");
    if(!f) { printf("Target commit not found.\n"); return; }
    while(fgets(line, sizeof(line), f)) {
        if(strncmp(line, "message:", 8) == 0 || strncmp(line, "timestamp:", 10) == 0 ||
           strncmp(line, "parent:", 7) == 0 || strncmp(line, "branch:", 7) == 0) continue;
        sscanf(line, "%127[^:]:%63s", filename, hash);
        enqueue(&merged, filename, hash);
    }
    fclose(f);

    // Load current commit files and check for conflicts
    snprintf(path, sizeof(path), "%s/%s", COMMITS_DIR, currentCommit);
    f = fopen(path, "r");
    if(!f) { printf("Current commit not found.\n"); return; }
    while(fgets(line, sizeof(line), f)) {
        if(strncmp(line, "message:", 8) == 0 || strncmp(line, "timestamp:", 10) == 0 ||
           strncmp(line, "parent:", 7) == 0 || strncmp(line, "branch:", 7) == 0) continue;
        sscanf(line, "%127[^:]:%63s", filename, hash);
        FileNode* node = findFile(&merged, filename);
        if(node) {
            if(strcmp(node->hash, hash) != 0)
                printf("CONFLICT: both modified %s\n", filename);
            strcpy(node->hash, hash); // prefer current
        } else {
            enqueue(&merged, filename, hash);
        }
    }
    fclose(f);

    // Write merged files to staging (index)
    f = fopen(INDEX_FILE, "w");
    FileNode* cur = merged.head;
    while(cur) {
        fprintf(f, "%s:%s\n", cur->filename, cur->hash);
        cur = cur->next;
    }
    fclose(f);

    printf("Merged target commit %s into current commit %s\n", targetCommit, currentCommit);

    // Free queue
    cur = merged.head;
    while(cur) {
        FileNode* tmp = cur;
        cur = cur->next;
        free(tmp);
    }
}