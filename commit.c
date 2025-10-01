#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MINI_GIT_DIR ".minigit"
#define COMMITS_DIR ".minigit/commits"
#define HEAD_FILE ".minigit/HEAD.txt"
#define BRANCHES_FILE ".minigit/branches.txt"
#define INDEX_FILE ".minigit/index.txt"
#define MAX_HASH 64
#define MAX_FILENAME 128

// Simple hash placeholder
char* Hash(const char* content) {
    static char hash[MAX_HASH];
    snprintf(hash, sizeof(hash), "%lu", (unsigned long)strlen(content));
    return hash;
}

// Linked list node for staging
typedef struct FileNode {
    char filename[MAX_FILENAME];
    char hash[MAX_HASH];
    struct FileNode* next;
} FileNode;

typedef struct {
    FileNode* head;
    FileNode* tail;
} FileQueue;

void initQueue(FileQueue* q) {
    q->head = q->tail = NULL;
}

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

void freeQueue(FileQueue* q) {
    FileNode* curr = q->head;
    while (curr) {
        FileNode* tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    q->head = q->tail = NULL;
}

// Utility: current timestamp
char* getCurrentTime() {
    static char buf[64];
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", t);
    return buf;
}

// Get current branch from HEAD file
char* getCurrentBranch() {
    static char branch[64];
    FILE* f = fopen(HEAD_FILE, "r");
    if (!f) return "main";  // default
    fgets(branch, sizeof(branch), f);
    branch[strcspn(branch, "\n")] = 0; // strip newline
    fclose(f);
    return branch;
}

// Get latest commit hash of a branch
char* getBranchHead(const char* branchName) {
    static char hash[MAX_HASH];
    FILE* f = fopen(BRANCHES_FILE, "r");
    if (!f) return "null";
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char branch[64], commit[128];
        sscanf(line, "%63[^:]:%127s", branch, commit);
        if (strcmp(branch, branchName) == 0) {
            fclose(f);
            return strdup(commit);
        }
    }
    fclose(f);
    return "null";
}

// Update branch head
void updateBranchHead(const char* branch, const char* newHash) {
    FILE* f = fopen(BRANCHES_FILE, "r");
    if (!f) return;

    char buffer[8192] = "";
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        char name[64], oldHash[128];
        sscanf(line, "%63[^:]:%127s", name, oldHash);
        if (strcmp(name, branch) == 0) {
            snprintf(line, sizeof(line), "%s:%s\n", branch, newHash);
        }
        strcat(buffer, line);
    }
    fclose(f);

    f = fopen(BRANCHES_FILE, "w");
    fputs(buffer, f);
    fclose(f);
}

// Commit function using queue
void commit(FileQueue* q, const char* message) {
    if (!q->head) {
        printf("There is nothing to commit.\n");
        return;
    }

    char* timestamp = getCurrentTime();
    char* branch = getCurrentBranch();
    char* parent = getBranchHead(branch);

    // Build commit metadata
    char metadata[1024];
    snprintf(metadata, sizeof(metadata),
             "message: %s\n"
             "timestamp: %s\n"
             "parent: %s\n"
             "branch: %s\n",
             message, timestamp, parent, branch);

    // Collect staged file entries
    char commitData[4096] = "";
    FileNode* curr = q->head;
    while (curr) {
        char entry[256];
        snprintf(entry, sizeof(entry), "%s:%s\n", curr->filename, curr->hash);
        strcat(commitData, entry);
        curr = curr->next;
    }

    // Compute commit hash
    char* commitHash = Hash(metadata + commitData);

    // Write commit file
    char commitPath[256];
    snprintf(commitPath, sizeof(commitPath), "%s/%s", COMMITS_DIR, commitHash);
    FILE* out = fopen(commitPath, "w");
    if (out) {
        fputs(metadata, out);
        fputs(commitData, out);
        fclose(out);
    }

    // Update branch head
    updateBranchHead(branch, commitHash);

    // Clear staging area
    freeQueue(q);

    // Reset index file
    FILE* idx = fopen(INDEX_FILE, "w");
    if (idx) fclose(idx);

    printf("Committed. Hash: %s\n", commitHash);
}