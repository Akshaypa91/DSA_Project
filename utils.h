// utils.h
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

// ===================== DATA STRUCTURES ===================== //

// --------- Queue / Linked List for Staging Area --------- //
typedef struct FileNode {
	char filename[100];
	struct FileNode* next;
} FileNode;

extern FileNode* stagingHead;
extern FileNode* stagingTail;

// --------- Commit Linked List (Graph-like) --------- //
typedef struct Commit {
	unsigned long commitId;
	char message[256];
	time_t timestamp;
	struct Commit* parent;
} Commit;

extern Commit* commitHead;

// --------- Branch Linked List --------- //
typedef struct Branch {
	char name[100];
	Commit* head;          // branch HEAD pointer
	struct Branch* next;
} Branch;

extern Branch* branchHead;

// ===================== FUNCTION PROTOTYPES ===================== //

// ---------- Utils (common) ---------- //
unsigned long simpleHash(char* str);
void makeDir(const char* folder);
char* readFile(const char* filename);
void writeFile(const char* filename, const char* content);
void printSeparator();

// ---------- Init ---------- //
void initRepo();

// ---------- Add (Queue / Linked List) ---------- //
void addFile(char* filename);
void showStagingArea();

// ---------- Commit ---------- //
void commitChanges(char* message);

// ---------- Log ---------- //
void logCommits();

// ---------- Checkout ---------- //
void checkoutCommit(unsigned long commitId);

// ---------- Branch ---------- //
void createBranch(char* name);
void listBranches();
void checkoutBranch(char* name);

// ---------- Merge ---------- //
void mergeBranch(char* branchName);

// ---------- Diff ---------- //
void diffCommits(unsigned long c1, unsigned long c2);

// ---------- Remote ---------- //
void pushToRemote(char* remotePath);
void pullFromRemote(char* remotePath);

#endif