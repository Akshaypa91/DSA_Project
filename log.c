// log.c
#include "utils.h"

// Traverse linked list of commits and print history
void logCommits() {
	if (commitHead == NULL) {
		printf("No commits yet.\n");
		return;
	}

	printSeparator();
	printf("Commit History:\n");

	Commit* temp = commitHead;
	while (temp != NULL) {
		printf("Commit ID: %lu\n", temp->commitId);
		printf("Message  : %s\n", temp->message);
		printf("Parent   : %p\n", (void*)temp->parent);
		printSeparator();
		temp = temp->parent;  // linked list backward traversal
	}
}