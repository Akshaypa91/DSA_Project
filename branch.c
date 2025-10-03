// branch.c
#include "utils.h"

// Create a new branch from current commit
void createBranch(char* name) {
	if (commitHead == NULL) {
		printf("No commits yet. Cannot create branch.\n");
		return;
	}

	Branch* newBranch = (Branch*)malloc(sizeof(Branch));
	strcpy(newBranch->name, name);
	newBranch->head = commitHead;   // branch points to current HEAD
	newBranch->next = branchHead;
	branchHead = newBranch;

	printf("Branch '%s' created at commit %lu\n", name, commitHead->commitId);
}

// List all branches
void listBranches() {
	if (branchHead == NULL) {
		printf("No branches created yet.\n");
		return;
	}

	printf("Branches:\n");
	Branch* temp = branchHead;
	while (temp != NULL) {
		if (temp->head == commitHead) {
			printf("* %s (HEAD)\n", temp->name);  // show current branch
		} else {
			printf("  %s\n", temp->name);
		}
		temp = temp->next;
	}
}

// Checkout a branch (switch HEAD to that branch)
void checkoutBranch(char* name) {
	Branch* temp = branchHead;
	while (temp != NULL) {
		if (strcmp(temp->name, name) == 0) {
			commitHead = temp->head;
			printf("Switched to branch '%s'\n", name);
			return;
		}
		temp = temp->next;
	}
	printf("Branch '%s' not found.\n", name);
}