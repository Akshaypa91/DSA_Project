#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OBJECTS_DIR "objects"
#define INDEX_FILE "index.txt"
#define MAX_HASH 64
#define MAX_FILENAME 128

// Dummy hash function (replace with real one)
char* Hash(const char* content) {
    static char hash[MAX_HASH];
    snprintf(hash, sizeof(hash), "%lu", (unsigned long)strlen(content));
    return hash;
}

// Node for linked list
typedef struct FileNode {
    char filename[MAX_FILENAME];
    char hash[MAX_HASH];
    struct FileNode* next;
} FileNode;

// Linked list to represent staging area
typedef struct {
    FileNode* head;
    FileNode* tail;
} FileQueue;

// Initialize queue
void initQueue(FileQueue* q) {
    q->head = q->tail = NULL;
}

// Push file into queue
void enqueue(FileQueue* q, const char* filename, const char* hash) {
    FileNode* node = (FileNode*)malloc(sizeof(FileNode));
    strcpy(node->filename, filename);
    strcpy(node->hash, hash);
    node->next = NULL;
    if (!q->head) {
        q->head = q->tail = node;
    } else {
        q->tail->next = node;
        q->tail = node;
    }
}

// Write queue to index file
void writeQueueToFile(FileQueue* q) {
    FILE* f = fopen(INDEX_FILE, "a");
    if (!f) return;

    FileNode* curr = q->head;
    while (curr) {
        fprintf(f, "%s:%s\n", curr->filename, curr->hash);
        curr = curr->next;
    }
    fclose(f);
}

// Free queue memory
void freeQueue(FileQueue* q) {
    FileNode* curr = q->head;
    while (curr) {
        FileNode* tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    q->head = q->tail = NULL;
}

// Add function using queue
void add(FileQueue* q, const char* filename) {
    FILE* inFile = fopen(filename, "r");
    if (!inFile) {
        printf("File not found: %s\n", filename);
        return;
    }

    fseek(inFile, 0, SEEK_END);
    long fileSize = ftell(inFile);
    rewind(inFile);

    char* content = (char*)malloc(fileSize + 1);
    if (!content) {
        printf("Memory allocation failed\n");
        fclose(inFile);
        return;
    }
    fread(content, 1, fileSize, inFile);
    content[fileSize] = '\0';
    fclose(inFile);

    char* hash = Hash(content);
    enqueue(q, filename, hash);

    // Also save the object file
    char outPath[256];
    snprintf(outPath, sizeof(outPath), "%s/%s", OBJECTS_DIR, hash);
    FILE* outFile = fopen(outPath, "w");
    if (outFile) {
        fwrite(content, 1, fileSize, outFile);
        fclose(outFile);
    }

    printf("Staged file: %s\n", filename);
    free(content);
}