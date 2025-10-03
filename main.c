// main.c
#include "utils.h"

// Global variables (defined once here)
FileNode* stagingHead = NULL;
FileNode* stagingTail = NULL;
Commit* commitHead = NULL;
Branch* branchHead = NULL;

int main() {
	int choice;
	char filename[100], msg[256], branchName[100], remotePath[200];
	unsigned long commitId, c1, c2;

	while (1) {
		printf("\n==== Mini Git (DSA Version Control) ====\n");
		printf("1. Init Repository\n");
		printf("2. Add File\n");
		printf("3. Show Staging Area\n");
		printf("4. Commit\n");
		printf("5. Log Commits\n");
		printf("6. Checkout Commit\n");
		printf("7. Create Branch\n");
		printf("8. List Branches\n");
		printf("9. Checkout Branch\n");
		printf("10. Merge Branch\n");
		printf("11. Diff Between Commits\n");
		printf("12. Push to Remote\n");
		printf("13. Pull from Remote\n");
		printf("0. Exit\n");
		printf("Enter choice: ");
		scanf("%d", &choice);
		getchar(); // clear newline

		switch (choice) {
			case 1:
				initRepo();
				break;

			case 2:
				printf("Enter filename: ");
				scanf("%s", filename);
				addFile(filename);
				break;

			case 3:
				showStagingArea();
				break;

			case 4:
				printf("Enter commit message: ");
				fgets(msg, sizeof(msg), stdin);
				msg[strcspn(msg, "\n")] = 0; // remove newline
				commitChanges(msg);
				break;

			case 5:
				logCommits();
				break;

			case 6:
				printf("Enter commit ID: ");
				scanf("%lu", &commitId);
				checkoutCommit(commitId);
				break;

			case 7:
				printf("Enter branch name: ");
				scanf("%s", branchName);
				createBranch(branchName);
				break;

			case 8:
				listBranches();
				break;

			case 9:
				printf("Enter branch name: ");
				scanf("%s", branchName);
				checkoutBranch(branchName);
				break;

			case 10:
				printf("Enter branch name to merge: ");
				scanf("%s", branchName);
				mergeBranch(branchName);
				break;

			case 11:
				printf("Enter Commit ID 1: ");
				scanf("%lu", &c1);
				printf("Enter Commit ID 2: ");
				scanf("%lu", &c2);
				diffCommits(c1, c2);
				break;

			case 12:
				printf("Enter remote folder path: ");
				scanf("%s", remotePath);
				pushToRemote(remotePath);
				break;

			case 13:
				printf("Enter remote folder path: ");
				scanf("%s", remotePath);
				pullFromRemote(remotePath);
				break;

			case 0:
				printf("Exiting Mini Git...\n");
				exit(0);

			default:
				printf("Invalid choice! Try again.\n");
		}
	}
	return 0;
}