#include "utils.h" //test2
Commit *commitHead = NULL;

#define MAX_MSG_LEN 256
#define MAX_ID_LEN  32

static int commitCount = 0;

// Generate Commit ID
static char* generateCommitID(char* message) {
	static char id[MAX_ID_LEN];

	unsigned long h1 = simpleHash(message);
	unsigned long h2 = (unsigned long)time(NULL);
	unsigned long h3 = commitCount * 9973; // spreading commits apart

	unsigned long combined = h1 ^ h2 ^ h3;

	snprintf(id, MAX_ID_LEN, "c%06lu", combined % 1000000);
	return id;
}

// Save snapshot of files
static void saveSnapshot(const char* commitID) {
	char folder[256];
	snprintf(folder, sizeof(folder), ".minigit/commits/%s", commitID);
	makeDir(".minigit/commits");
	makeDir(folder);

	FILE* f = fopen(".minigit/staging.txt", "r");
	if (!f) {
		printf("Nothing to commit! (staging empty)\n");
		return;
	}

	char line[256];
	while (fgets(line, sizeof(line), f)) {
		size_t len = strlen(line);
		if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
			line[len-1] = '\0';

		char* content = readFile(line);
		if (!content) {
			printf("Warning: could not read %s\n", line);
			continue;
		}

		char path[512];
		snprintf(path, sizeof(path), "%s/%s", folder, line);
		writeFile(path, content);
		free(content);
	}
	fclose(f);
}

// Make a new commit
void commitChanges(char* msg) {
	FILE* f = fopen(".minigit/staging.txt", "r");
	if (!f) {
		printf("Staging area is empty. Nothing to commit.\n");
		return;
	}
	fclose(f);

	char* id = generateCommitID(msg);

	// Create linked list node
	Commit* newCommit = (Commit*)malloc(sizeof(Commit));
	newCommit->commitId = simpleHash(id);
	strncpy(newCommit->message, msg, sizeof(newCommit->message));
	newCommit->timestamp = time(NULL);
	newCommit->parent = commitHead;

	commitHead = newCommit;

	saveSnapshot(id);

	// Append commit info to commits.txt
	FILE* logFile = fopen(".minigit/commits.txt", "a");
	if (logFile) {
		fprintf(logFile, "%s : %s : %lld\n", id, msg, (long long)newCommit->timestamp);
		fclose(logFile);
	}

	// Clear staging file
	f = fopen(".minigit/staging.txt", "w");
	if (f) fclose(f);

	commitCount++;
	printf("Committed as %s : %s\n", id, msg);
}