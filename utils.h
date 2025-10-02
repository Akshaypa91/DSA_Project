// utils.h
#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// ==== Init (HashMap based tracking) ==== //
void initRepo();
void trackFile(char* filename);
void printTrackedFiles();

// ==== Add (Queue / Linked List staging area) ==== //
void enqueueFile(char* filename);
void dequeueFile();
void printStagingArea();

// ==== Commit (Linked List + Hashing) ==== //
void makeCommit(char* msg);
void printLog();

// ==== Checkout (Stack) ==== //
void pushCommit(char* commitID);
char* popCommit();
void checkoutCommit(char* commitID);

// ==== Branch (Graph) ==== //
typedef struct Graph Graph;
Graph* createGraph(int V);
void addEdge(Graph* g, int u, int v);
void printGraph(Graph* g);

// ==== Diff (LCS) ==== //
int LCS(char* a, char* b);

#endif