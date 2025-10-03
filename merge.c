// merge.c
#include "utils.h"

#define MAX_BRANCH 20
#define MAX_CHILD  10
#define MAX_NAME   50

// ==== Graph Node: Commit ==== //
typedef struct CommitNode {
	char commitID[32];
	char message[256];
	struct CommitNode* parent;
	struct CommitNode* children[MAX_CHILD];
	int childCount;
} CommitNode;

// ==== Branch Structure ==== //
typedef struct Branch {
	char name[MAX_NAME];
	CommitNode* head;
} Branch;

static Branch branches[MAX_BRANCH];
static int branchCount = 0;
static Branch* currentBranch = NULL;

// ==== Create new commit node ==== //
CommitNode* createCommitNode(const char* id, const char* msg, CommitNode* parent) {
	CommitNode* node = (CommitNode*)malloc(sizeof(CommitNode));
	strcpy(node->commitID, id);
	strcpy(node->message, msg);
	node->parent = parent;
	node->childCount = 0;

	if (parent != NULL && parent->childCount < MAX_CHILD) {
		parent->children[parent->childCount++] = node;
	}
	return node;
}

// ==== Create new branch ==== //
void createBranch(const char* name, const char* baseCommitID, const char* msg) {
	if (branchCount >= MAX_BRANCH) {
		printf("Max branches reached!\n");
		return;
	}

	CommitNode* root = createCommitNode(baseCommitID, msg, NULL);
	strcpy(branches[branchCount].name, name);
	branches[branchCount].head = root;

	if (currentBranch == NULL) {
		currentBranch = &branches[branchCount];
	}
	branchCount++;
	printf("Branch '%s' created at commit %s\n", name, baseCommitID);
}

// ==== Switch branch ==== //
void switchBranch(const char* name) {
	for (int i = 0; i < branchCount; i++) {
		if (strcmp(branches[i].name, name) == 0) {
			currentBranch = &branches[i];
			printf("Switched to branch %s\n", name);
			return;
		}
	}
	printf("Branch %s not found!\n", name);
}

// ==== Merge branches ==== //
void mergeBranches(const char* branch1, const char* branch2) {
	Branch *b1 = NULL, *b2 = NULL;
	for (int i = 0; i < branchCount; i++) {
		if (strcmp(branches[i].name, branch1) == 0) b1 = &branches[i];
		if (strcmp(branches[i].name, branch2) == 0) b2 = &branches[i];
	}
	if (!b1 || !b2) {
		printf("One or both branches not found!\n");
		return;
	}

	// simple merge = create new commit with both parents
	char mergedID[32];
	snprintf(mergedID, sizeof(mergedID), "M%d", rand() % 10000);

	char msg[256];
	snprintf(msg, sizeof(msg), "Merged branch %s and %s", branch1, branch2);

	CommitNode* mergeCommit = createCommitNode(mergedID, msg, b1->head);
	// also link b2 as parent (simulate multi-parent commit)
	if (mergeCommit->childCount < MAX_CHILD) {
		mergeCommit->children[mergeCommit->childCount++] = b2->head;
	}

	b1->head = mergeCommit;
	printf("Branches %s and %s merged into commit %s\n", branch1, branch2, mergedID);
}

// ==== Print branch commits (DFS) ==== //
void printBranchCommits(CommitNode* node, int depth) {
	if (!node) return;
	for (int i = 0; i < depth; i++) printf("  ");
	printf("%s : %s\n", node->commitID, node->message);
	for (int i = 0; i < node->childCount; i++) {
		printBranchCommits(node->children[i], depth + 1);
	}
}

// ==== Show all branches ==== //
void listBranches() {
	printf("Branches:\n");
	for (int i = 0; i < branchCount; i++) {
		printf("- %s\n", branches[i].name);
	}
}

// ==== Show commits in current branch ==== //
void showCurrentBranchCommits() {
	if (!currentBranch) {
		printf("No active branch!\n");
		return;
	}
	printf("Commits in branch %s:\n", currentBranch->name);
	printBranchCommits(currentBranch->head, 0);
}