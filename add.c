// add.c
#include "utils.h"

#define MAX_FILENAME_LEN 256
#define STAGING_FILE ".minigit/staging.txt"

// ===== Queue node for staging area =====
typedef struct QNode {
	char filename[MAX_FILENAME_LEN];
	struct QNode* next;
} QNode;

static QNode* front = NULL;
static QNode* rear  = NULL;

// ===== Internal helpers for persistence =====
static void appendToStagingFile(const char* filename) {
	FILE* f = fopen(STAGING_FILE, "a");
	if (!f) return;
	fprintf(f, "%s\n", filename);
	fclose(f);
}

static void rewriteStagingFileFromQueue() {
	FILE* f = fopen(STAGING_FILE, "w");
	if (!f) return;
	QNode* cur = front;
	while (cur) {
		fprintf(f, "%s\n", cur->filename);
		cur = cur->next;
	}
	fclose(f);
}

static void loadStagingFromFile() {
	// Clear current in-memory queue
	while (front) {
		QNode* tmp = front;
		front = front->next;
		free(tmp);
	}
	rear = NULL;

	FILE* f = fopen(STAGING_FILE, "r");
	if (!f) return; // no staging file yet

	char line[MAX_FILENAME_LEN];
	while (fgets(line, sizeof(line), f)) {
		// trim newline
		size_t len = strlen(line);
		if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
			line[len-1] = '\0';
			len--;
			if (len > 0 && line[len-1] == '\r') { line[len-1] = '\0'; }
		}
		// enqueue without writing back to file
		QNode* node = (QNode*)malloc(sizeof(QNode));
		strncpy(node->filename, line, MAX_FILENAME_LEN);
		node->filename[MAX_FILENAME_LEN-1] = '\0';
		node->next = NULL;
		if (rear == NULL) {
			front = rear = node;
		} else {
			rear->next = node;
			rear = node;
		}
	}
	fclose(f);
}

// ===== Public API =====

// Add file to staging area (enqueue)
void enqueueFile(char* filename) {
	if (!filename || strlen(filename) == 0) {
		printf("enqueueFile: invalid filename\n");
		return;
	}

	// ensure .minigit folder exists
	makeDir(".minigit");

	// load current staging (keeps in-memory consistent if process restarted)
	loadStagingFromFile();

	// create new node
	QNode* node = (QNode*)malloc(sizeof(QNode));
	strncpy(node->filename, filename, MAX_FILENAME_LEN);
	node->filename[MAX_FILENAME_LEN-1] = '\0';
	node->next = NULL;

	if (rear == NULL) {
		front = rear = node;
	} else {
		rear->next = node;
		rear = node;
	}

	// persist by appending to staging file
	appendToStagingFile(filename);

	// optionally track file in hash map (so tracked files table is updated)
	trackFile(filename);

	printf("Added '%s' to staging area\n", filename);
}

// Remove and return front filename from staging (dequeue)
// Note: returned string is malloc'ed; caller should free it.
void dequeueFile() {
	// load current staging first
	loadStagingFromFile();

	if (front == NULL) {
		printf("Staging area is empty\n");
		return;
	}

	QNode* temp = front;
	front = front->next;
	if (front == NULL) rear = NULL;

	// rewrite staging file to reflect removal
	rewriteStagingFileFromQueue();

	printf("Removed '%s' from staging area\n", temp->filename);
	free(temp);
}

// Print all files currently in staging area
void printStagingArea() {
	// ensure we display latest persisted state
	loadStagingFromFile();

	if (front == NULL) {
		printf("Staging area is empty\n");
		return;
	}

	printf("Staging area:\n");
	QNode* cur = front;
	int idx = 1;
	while (cur) {
		printf("  %d. %s\n", idx++, cur->filename);
		cur = cur->next;
	}
}