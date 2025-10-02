// commit.c
#include "utils.h"

#define MAX_MSG_LEN 256
#define MAX_ID_LEN  32

// ==== Commit structure (Linked List) ==== //
typedef struct Commit {
	char id[MAX_ID_LEN];
	char message[MAX_MSG_LEN];
	struct Commit* next;
} Commit;

static Commit* commitHead = NULL;   // linked list head
static int commitCount = 0;

// ==== Generate Commit ID ==== //
static char* generateCommitID(char* message) {
	static char id[MAX_ID_LEN];
	unsigned long h = simpleHash(message) ^ (commitCount + 1) * 131; 
	snprintf(id, MAX_ID_LEN, "c%lu", h % 1000000);
	return id;
}

// ==== Save snapshot of files ==== //
static void saveSnapshot(const char* commitID) {
	char folder[256];
	snprintf(folder, sizeof(folder), ".minigit/commits/%s", commitID);
	makeDir(".minigit/commits");
	makeDir(folder);

	// Load staging area
	FILE* f = fopen(".minigit/staging.txt", "r");
	if (!f) {
		printf("Nothing to commit! (staging empty)\n");
		return;
	}

	char line[256];
	while (fgets(line, sizeof(line), f)) {
		// trim newline
		size_t len = strlen(line);
		if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
			line[len-1] = '\0';

		char* content = readFile(line);
		if (!content) {
			printf("Warning: could not read %s\n", line);
			continue;
		}

		// create path inside commit folder
		char path[512];
		snprintf(path, sizeof(path), "%s/%s", folder, line);

		// ensure subfolder path exists (simple: only top-level supported now)
		writeFile(path, content);
		free(content);
	}
	fclose(f);
}

// ==== Make a new commit ==== //
void makeCommit(char* msg) {
	// check staging
	FILE* f = fopen(".minigit/staging.txt", "r");
	if (!f) {
		printf("Staging area is empty. Nothing to commit.\n");
		return;
	}
	fclose(f);

	// Generate ID
	char* id = generateCommitID(msg);

	// Create linked list node
	Commit* newCommit = (Commit*)malloc(sizeof(Commit));
	strncpy(newCommit->id, id, MAX_ID_LEN);
	strncpy(newCommit->message, msg, MAX_MSG_LEN);
	newCommit->next = commitHead;
	commitHead = newCommit;

	// Save snapshot
	saveSnapshot(id);

	// Append commit info to commits.txt
	FILE* logFile = fopen(".minigit/commits.txt", "a");
	if (logFile) {
		fprintf(logFile, "%s : %s\n", id, msg);
		fclose(logFile);
	}

	// Clear staging file (queue flushed after commit)
	f = fopen(".minigit/staging.txt", "w");
	if (f) fclose(f);

	commitCount++;
	printf("Committed as %s : %s\n", id, msg);
}

// ==== Print commit history (linked list traversal) ==== //
void printLog() {
	if (!commitHead) {
		// If in-memory empty, reload from commits.txt
		FILE* f = fopen(".minigit/commits.txt", "r");
		if (!f) {
			printf("No commits yet.\n");
			return;
		}
		char line[512];
		printSeparator();
		while (fgets(line, sizeof(line), f)) {
			printf("%s", line);
		}
		printSeparator();
		fclose(f);
		return;
	}

	// Print from linked list (most recent first)
	Commit* temp = commitHead;
	printSeparator();
	while (temp) {
		printf("%s : %s\n", temp->id, temp->message);
		temp = temp->next;
	}
	printSeparator();
}