#include "utils.h"

#define REMOTE_TABLE_SIZE 100

// Queue node for files waiting to be pushed
typedef struct PushQueue {
    char filename[100];
    struct PushQueue* next;
} PushQueue;

PushQueue* pushFront = NULL;
PushQueue* pushRear = NULL;

// Hash table to simulate files on remote repository
FileNode* remoteHashTable[REMOTE_TABLE_SIZE];

// Hash function for remote files
int remoteHashFunc(const char* str) {
    int hash = 0;
    for (int i = 0; str[i]; i++) {
        hash = (hash * 31 + str[i]) % REMOTE_TABLE_SIZE;
    }
    return hash;
}

// Add file to remote hash table
void addToRemote(const char* filename) {
    int idx = remoteHashFunc(filename);
    FileNode* newNode = (FileNode*)malloc(sizeof(FileNode));
    strcpy(newNode->filename, filename);
    newNode->next = remoteHashTable[idx];
    remoteHashTable[idx] = newNode;
}

// Check if file already exists on remote
int existsOnRemote(const char* filename) {
    int idx = remoteHashFunc(filename);
    FileNode* temp = remoteHashTable[idx];
    while (temp) {
        if (strcmp(temp->filename, filename) == 0)
            return 1;
        temp = temp->next;
    }
    return 0;
}

// Add file to push queue
void enqueueFile(const char* filename) {
    PushQueue* newNode = (PushQueue*)malloc(sizeof(PushQueue));
    strcpy(newNode->filename, filename);
    newNode->next = NULL;

    if (!pushRear)
        pushFront = pushRear = newNode;
    else {
        pushRear->next = newNode;
        pushRear = newNode;
    }
}

// Remove file from push queue
char* dequeueFile() {
    if (!pushFront) return NULL;
    PushQueue* temp = pushFront;
    pushFront = pushFront->next;
    if (!pushFront) pushRear = NULL;
    char* file = strdup(temp->filename);
    free(temp);
    return file;
}

// Push files from local to remote
void enqueuePush(const char* commitID) {
    printf("Pushing commit %s to remote...\n", commitID);

    // Add files from staging area to queue if not already on remote
    FileNode* temp = stagingHead;
    while (temp) {
        if (!existsOnRemote(temp->filename)) {
            enqueueFile(temp->filename);
            addToRemote(temp->filename);
            printf("Queued '%s' for push.\n", temp->filename);
        } else {
            printf("Skipping '%s' (already on remote).\n", temp->filename);
        }
        temp = temp->next;
    }

    // Push queued files to remote
    char* pushedFile;
    while ((pushedFile = dequeueFile()) != NULL) {
        printf("Pushed file: %s\n", pushedFile);
        free(pushedFile);
    }

    printf("Push complete for commit %s.\n", commitID);
}

// Pull files from remote to local
void pullCommit(const char* commitID) {
    printf("Pulling commit %s from remote...\n", commitID);

    // Retrieve all files stored in remote hash table
    for (int i = 0; i < REMOTE_TABLE_SIZE; i++) {
        FileNode* temp = remoteHashTable[i];
        while (temp) {
            printf("Pulled file: %s\n", temp->filename);
            temp = temp->next;
        }
    }

    printf("Pull complete for commit %s.\n", commitID);
}
