#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> //for Time Function

//For Other Operating Systems
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

typedef struct FileNode {
    char filename[100];
    struct FileNode* next;
} FileNode;

typedef struct Commit {
    unsigned long commitId;
    char message[256];
    time_t timestamp;
    struct Commit* parent;
} Commit;

typedef struct Branch {
    char name[100];
    Commit* head;
    struct Branch* next;
} Branch;

// Global Variable
extern FileNode* stagingHead;
extern FileNode* stagingTail;
extern Commit* commitHead;
extern Branch* branchHead;

// Utils Function ProtoTypes
unsigned long simpleHash(char* str);
void makeDir(const char* folder);
char* readFile(const char* filename);
void writeFile(const char* filename, const char* content);
void printSeparator();
int fileExists(const char* filename);
void trackFile(const char* filename);

// Function Module
// Init
void initRepo();

// Add
void addFile(char* filename);
void showStagingArea(void);

// Commit
void commitChanges(char* message);

// Log
void logCommits();

// Checkout
void checkoutCommit(unsigned long commitId);

// Branch
void createBranch(char* name);
void listBranches();
void checkoutBranch(char* name);

// Merge
void mergeBranch(char* branchName);

// Diff
void diffCommits(unsigned long c1, unsigned long c2);

// Remote
void pushToRemote(char* remotePath);
void pullFromRemote(char* remotePath);

#endif