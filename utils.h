#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

// ======= DATA STRUCTURES ======= //

// Hash table for tracking files
#define TABLE_SIZE 100
extern struct FileNode* hashTable[TABLE_SIZE];

// Forward declaration of hash function
int hashFunc(const char* str);
void trackFile(const char* filename);


// File node for tracking files (hash table in utils.c)
typedef struct FileNode {
    char filename[100];
    struct FileNode* next;
} FileNode;

// Global pointers for staging area (used in add.c)
extern FileNode* stagingHead;
extern FileNode* stagingTail;

// Commit structure
typedef struct Commit {
    unsigned long commitId;
    char message[256];
    time_t timestamp;
    struct Commit* parent;
} Commit;

// Global pointer for commit list (used in commit.c, log.c)
extern Commit* commitHead;

// Branch structure
typedef struct Branch {
    char name[100];
    Commit* head;
    struct Branch* next;
} Branch;

// Global pointer for branch list (used in branch.c)
extern Branch* branchHead;

// ======= UTILS FUNCTION PROTOTYPES ======= //

// General helpers
unsigned long simpleHash(char* str);
void makeDir(const char* folder);
char* readFile(const char* filename);
void writeFile(const char* filename, const char* content);
void printSeparator();
int fileExists(const char* filename);
void trackFile(const char* filename);  // only in utils.c

// ======= MODULE FUNCTIONS ======= //

// Init
void initRepo(); // defined in init.c

// Add
void addFile(char* filename);          // defined in add.c
void showStagingArea(void);            // defined in add.c

// Commit
void commitChanges(char* message);     // defined in commit.c

// Log
void logCommits();                     // defined in log.c

// Checkout
void checkoutCommit(unsigned long commitId); // defined in checkout.c

// Branch
void createBranch(char* name);         // defined in branch.c
void listBranches();                    // defined in branch.c
void checkoutBranch(char* name);       // defined in branch.c

// Merge
void mergeBranch(char* branchName);    // defined in merge.c

// Diff
void diffCommits(unsigned long c1, unsigned long c2); // defined in diff.c

// Remote
void pushToRemote(char* remotePath);   // defined in remote.c
void pullFromRemote(char* remotePath); // defined in remote.c

#endif // UTILS_H