// add.c //test FeatureBranch
#include "utils.h"

#define MAX_FILENAME_LEN 256
#define STAGING_FILE ".minigit/staging.txt"

// Forward declarations
void enqueueFile(const char* filename);
void printStagingArea(void);

// Queue node for Staging Area
typedef struct QNode {
	char filename[MAX_FILENAME_LEN];
	struct QNode* next;
} QNode;

static QNode* front = NULL;
static QNode* rear  = NULL;

// Internal helpers
static void appendToStagingFile(const char* filename) {
	FILE* f = fopen(STAGING_FILE, "a");
	if (!f) return;
	fprintf(f, "%s\n", filename);
	fclose(f);
}

static void rewriteStagingFileFromQueue() {
	FILE* f = fopen(STAGING_FILE, "w");
	if (!f) return;
	QNode* curr = front;
	while (curr) {
		fprintf(f, "%s\n", curr->filename);
		curr = curr->next;
	}
	fclose(f);
}

static void loadStagingFromFile() {
	// Clear current queue
	while (front) {
		QNode* tmp = front;
		front = front->next;
		free(tmp);
	}
	rear = NULL;

	FILE* f = fopen(STAGING_FILE, "r");
	if (!f) return;

	char line[MAX_FILENAME_LEN];
	while (fgets(line, sizeof(line), f)) {
		// trim newline
		size_t len = strlen(line);
		if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
			line[len-1] = '\0';
			len--;
			if (len > 0 && line[len-1] == '\r') {
				line[len-1] = '\0';
			}
		}

		QNode* node = (QNode*)malloc(sizeof(QNode));
		strncpy(node->filename, line, MAX_FILENAME_LEN);
		node->filename[MAX_FILENAME_LEN-1] = '\0';
		node->next = NULL;

		if (rear == NULL) {
			front = rear = node;
		}
		else {
			rear->next = node;
			rear = node;
		}
	}
	fclose(f);
}

// Public API
void enqueueFile(const char* filename) {
	if (!filename || strlen(filename) == 0) {
		printf("enqueueFile: invalid filename\n");
		return;
	}

	makeDir(".minigit");
	loadStagingFromFile();

	QNode* node = (QNode*)malloc(sizeof(QNode));
	strncpy(node->filename, filename, MAX_FILENAME_LEN);
	node->filename[MAX_FILENAME_LEN-1] = '\0';
	node->next = NULL;

	if (rear == NULL) {
		front = rear = node;
	}
	else {
		rear->next = node;
		rear = node;
	}

	appendToStagingFile(filename);
	trackFile(filename); //track file globally

	printf("Added '%s' to staging area\n", filename);
}

void dequeueFile(void) {
	loadStagingFromFile();
	if (!front) {
		printf("Staging area is empty\n");
		return;
	}

	QNode* temp = front;
	front = front->next;
	if (!front) {
		rear = NULL;
	}

	rewriteStagingFileFromQueue();
	printf("Removed '%s' from staging area\n", temp->filename);
	free(temp);
}

void printStagingArea(void) {
	loadStagingFromFile();
	if (!front) {
		printf("Staging area is empty\n");
		return;
	}

	printf("Staging area:\n");
	QNode* curr = front;
	int idx = 1;
	while (curr) {
		printf("  %d. %s\n", idx++, curr->filename);
		curr = curr->next;
	}
}

// Optional--> wrapper functions called by main.c
void addFile(char* filename) {
	enqueueFile(filename);
}
void showStagingArea(void) {
	printStagingArea();
}