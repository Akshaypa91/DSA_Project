// remote.c
#include "utils.h"
#include <dirent.h>
#include <errno.h>

#define REMOTE_DIR        ".minigit/remote_repo"
#define REMOTE_QUEUE_FILE ".minigit/remote_queue.txt"
#define MAX_ID_LEN        64
#define MAX_PATH_LEN      512

// ===== Simple in-memory queue node for commits to push =====
typedef struct PushNode {
	char commitID[MAX_ID_LEN];
	struct PushNode* next;
} PushNode;

static PushNode* rq_front = NULL;
static PushNode* rq_rear  = NULL;

// ===== Helpers for queue persistence =====
static void appendToRemoteQueueFile(const char* commitID) {
	FILE* f = fopen(REMOTE_QUEUE_FILE, "a");
	if (!f) return;
	fprintf(f, "%s\n", commitID);
	fclose(f);
}

static void rewriteRemoteQueueFileFromQueue() {
	FILE* f = fopen(REMOTE_QUEUE_FILE, "w");
	if (!f) return;
	PushNode* cur = rq_front;
	while (cur) {
		fprintf(f, "%s\n", cur->commitID);
		cur = cur->next;
	}
	fclose(f);
}

static void loadRemoteQueueFromFile() {
	// clear in-memory queue
	while (rq_front) {
		PushNode* t = rq_front;
		rq_front = rq_front->next;
		free(t);
	}
	rq_rear = NULL;

	FILE* f = fopen(REMOTE_QUEUE_FILE, "r");
	if (!f) return;

	char line[MAX_ID_LEN];
	while (fgets(line, sizeof(line), f)) {
		// trim newline
		size_t len = strlen(line);
		if (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
			line[len-1] = '\0';
			len--;
			if (len > 0 && line[len-1] == '\r') line[len-1] = '\0';
		}
		PushNode* node = (PushNode*)malloc(sizeof(PushNode));
		strncpy(node->commitID, line, MAX_ID_LEN);
		node->commitID[MAX_ID_LEN-1] = '\0';
		node->next = NULL;
		if (rq_rear == NULL) {
			rq_front = rq_rear = node;
		} else {
			rq_rear->next = node;
			rq_rear = node;
		}
	}
	fclose(f);
}

// ===== FS helpers =====
static int dirExists(const char* path) {
	struct stat st;
	if (stat(path, &st) != 0) return 0;
	return S_ISDIR(st.st_mode);
}

static int fileExists(const char* path) {
	struct stat st;
	return (stat(path, &st) == 0 && S_ISREG(st.st_mode));
}

static int copyFileContents(const char* src, const char* dst) {
	char* content = readFile(src);
	if (!content) return 0;
	// ensure target folder exists (caller should create folder)
	writeFile(dst, content);
	free(content);
	return 1;
}

// Copy all files from srcFolder/* to dstFolder/*
static void copyFolderFiles(const char* srcFolder, const char* dstFolder) {
	DIR* d = opendir(srcFolder);
	if (!d) return;
	makeDir(dstFolder); // ensure destination exists
	struct dirent* de;
	while ((de = readdir(d)) != NULL) {
		// skip . and ..
		if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;

		char srcPath[MAX_PATH_LEN];
		char dstPath[MAX_PATH_LEN];
		snprintf(srcPath, sizeof(srcPath), "%s/%s", srcFolder, de->d_name);
		snprintf(dstPath, sizeof(dstPath), "%s/%s", dstFolder, de->d_name);

		// Only copy regular files (skip directories in this simple impl)
		struct stat st;
		if (stat(srcPath, &st) == 0 && S_ISREG(st.st_mode)) {
			copyFileContents(srcPath, dstPath);
		}
	}
	closedir(d);
}

// ===== Public API =====

// Enqueue a commit ID to be pushed to remote later.
// Persisted in .minigit/remote_queue.txt
void enqueuePush(const char* commitID) {
	if (!commitID || strlen(commitID) == 0) {
		printf("enqueuePush: invalid commitID\n");
		return;
	}

	// ensure .minigit folder exists
	makeDir(".minigit");

	// load current queue
	loadRemoteQueueFromFile();

	// append node
	PushNode* node = (PushNode*)malloc(sizeof(PushNode));
	strncpy(node->commitID, commitID, MAX_ID_LEN);
	node->commitID[MAX_ID_LEN-1] = '\0';
	node->next = NULL;
	if (rq_rear == NULL) {
		rq_front = rq_rear = node;
	} else {
		rq_rear->next = node;
		rq_rear = node;
	}

	appendToRemoteQueueFile(commitID);
	printf("Enqueued commit %s for push\n", commitID);
}

// Process the push queue: copy commits to .minigit/remote_repo/<commitID>/
void processPushQueue() {
	// ensure remote dir exists
	makeDir(REMOTE_DIR);

	// load queue
	loadRemoteQueueFromFile();

	if (rq_front == NULL) {
		printf("Remote push queue is empty\n");
		return;
	}

	while (rq_front) {
		PushNode* node = rq_front;
		rq_front = rq_front->next;
		if (rq_front == NULL) rq_rear = NULL;

		// local commit folder
		char localCommitFolder[MAX_PATH_LEN];
		snprintf(localCommitFolder, sizeof(localCommitFolder), ".minigit/commits/%s", node->commitID);

		if (!dirExists(localCommitFolder)) {
			printf("Warning: local commit %s not found, skipping\n", node->commitID);
			free(node);
			continue;
		}

		// remote commit folder
		char remoteCommitFolder[MAX_PATH_LEN];
		snprintf(remoteCommitFolder, sizeof(remoteCommitFolder), "%s/%s", REMOTE_DIR, node->commitID);

		// copy files
		copyFolderFiles(localCommitFolder, remoteCommitFolder);

		// append to remote commits log for visibility
		char remoteLog[MAX_PATH_LEN];
		snprintf(remoteLog, sizeof(remoteLog), "%s/remote_commits.txt", REMOTE_DIR);
		FILE* rf = fopen(remoteLog, "a");
		if (rf) {
			fprintf(rf, "%s\n", node->commitID);
			fclose(rf);
		}

		printf("Pushed commit %s to remote\n", node->commitID);
		free(node);

		// rewrite queue file from remaining queue
		rewriteRemoteQueueFileFromQueue();
	}
	// ensure queue file is empty now
	FILE* qf = fopen(REMOTE_QUEUE_FILE, "w");
	if (qf) fclose(qf);
}

// List commits present on remote repository
void listRemoteCommits() {
	char remoteLog[MAX_PATH_LEN];
	snprintf(remoteLog, sizeof(remoteLog), "%s/remote_commits.txt", REMOTE_DIR);
	FILE* rf = fopen(remoteLog, "r");
	if (!rf) {
		printf("No commits on remote yet.\n");
		return;
	}
	printf("Remote commits:\n");
	char line[128];
	int idx = 1;
	while (fgets(line, sizeof(line), rf)) {
		// trim newline
		line[strcspn(line, "\n")] = '\0';
		printf("  %d. %s\n", idx++, line);
	}
	fclose(rf);
}

// Pull a commit from remote into local .minigit/commits/<commitID>/
void pullCommit(const char* commitID) {
	if (!commitID || strlen(commitID) == 0) {
		printf("pullCommit: invalid commitID\n");
		return;
	}

	char remoteCommitFolder[MAX_PATH_LEN];
	snprintf(remoteCommitFolder, sizeof(remoteCommitFolder), "%s/%s", REMOTE_DIR, commitID);

	if (!dirExists(remoteCommitFolder)) {
		printf("Commit %s not found on remote\n", commitID);
		return;
	}

	char localCommitFolder[MAX_PATH_LEN];
	snprintf(localCommitFolder, sizeof(localCommitFolder), ".minigit/commits/%s", commitID);
	makeDir(".minigit/commits");
	makeDir(localCommitFolder);

	copyFolderFiles(remoteCommitFolder, localCommitFolder);

	// Add to local commits log (if not already present)
	FILE* lf = fopen(".minigit/commits.txt", "a");
	if (lf) {
		fprintf(lf, "%s : pulled-from-remote\n", commitID);
		fclose(lf);
	}
	printf("Pulled commit %s from remote into local repo\n", commitID);
}