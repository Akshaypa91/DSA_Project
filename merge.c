#include "utils.h"

void mergeBranch(char* branchName) {
	Branch* target = branchHead;
	Branch* mergeFrom = NULL;

	// find target branch
	while (target != NULL) {
		if (strcmp(target->name, branchName) == 0) {
			mergeFrom = target;
			break;
		}
		target = target->next;
	}

	if (!mergeFrom) {
		printf("Branch '%s' not found!\n", branchName);
		return;
	}

	if (!commitHead || !mergeFrom->head) {
		printf("Nothing to merge!\n");
		return;
	}

	// Simple merge => copy latest commit message
	Commit* newCommit = (Commit*)malloc(sizeof(Commit));
	newCommit->commitId = simpleHash("merge");
	snprintf(newCommit->message, sizeof(newCommit->message),
	         "Merged branch '%s' into current branch", branchName);
	newCommit->timestamp = time(NULL);
	newCommit->parent = commitHead;

	commitHead = newCommit;
	printf("Merged branch '%s' into current branch successfully!\n", branchName);
}