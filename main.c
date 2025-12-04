// main.c
#include "utils.h"
//test2 //test3
int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Usage: ./mini_git <command> [args]\n");
		printf("Commands: init, add, show, commit, log, checkout, branch, merge, diff, push, pull\n");
		return 1;
	}

	char* cmd = argv[1];

	if (strcmp(cmd, "init") == 0) {
		initRepo();
	}
	else if (strcmp(cmd, "add") == 0) {
		if (argc < 3) {
			printf("Usage: ./mini_git add <filename>\n");
			return 1;
		}
		addFile(argv[2]);
	}
	else if (strcmp(cmd, "show") == 0) {
		showStagingArea();
	}
	else if (strcmp(cmd, "commit") == 0) {
		if (argc < 3) {
			printf("Usage: ./mini_git commit <message>\n");
			return 1;
		}
		commitChanges(argv[2]);
	}
	else if (strcmp(cmd, "log") == 0) {
		logCommits();
	}
	else if (strcmp(cmd, "checkout") == 0) {
		if (argc < 3) {
			printf("Usage: ./mini_git checkout <commitID>\n");
			return 1;
		}
		checkoutCommit(argv[2]);   // pass commit ID as STRING
	}
	else if (strcmp(argv[1], "branch") == 0) {
		if (argc == 2) {
			listBranches();
		} else {
			createBranch(argv[2]);
		}
	}

	else if (strcmp(cmd, "list-branches") == 0) {
		listBranches();
	}
	else if (strcmp(cmd, "checkout-branch") == 0) {
		if (argc < 3) {
			printf("Usage: ./mini_git checkout-branch <branchName>\n");
			return 1;
		}
		checkoutBranch(argv[2]);
	}
	else if (strcmp(cmd, "merge") == 0) {
		if (argc < 3) {
			printf("Usage: ./mini_git merge <branchName>\n");
			return 1;
		}
		mergeBranch(argv[2]);
	}
	else if (strcmp(cmd, "diff") == 0) {
		if (argc < 4) {
			printf("Usage: ./mini_git diff <commit1> <commit2>\n");
			return 1;
		}
		diffCommits(strtoul(argv[2], NULL, 10), strtoul(argv[3], NULL, 10));
	}
	else if (strcmp(cmd, "push") == 0) {
		if (argc < 3) {
			printf("Usage: ./mini_git push <remotePath>\n");
			return 1;
		}
		pushToRemote(argv[2]);
	}
	else if (strcmp(cmd, "pull") == 0) {
		if (argc < 3) {
			printf("Usage: ./mini_git pull <remotePath>\n");
			return 1;
		}
		pullFromRemote(argv[2]);
	}
	else {
		printf("Unknown command: %s\n", cmd);
	}

	return 0;
}