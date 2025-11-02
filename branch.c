// branch.c //test2
#include "utils.h"


// Restore last commit if memory is empty
void restoreLastCommit() {
    if (commitHead != NULL) return;

    FILE* f = fopen(".minigit/commits.txt", "r");
    if (!f) return;

    char id[64], msg[256];
    time_t timestamp;
    Commit* last = NULL;

    while (fscanf(f, "%s : %[^:] : %lld\n", id, msg, &timestamp) == 3) {
        Commit* c = (Commit*)malloc(sizeof(Commit));
        c->commitId = simpleHash(id);
        strcpy(c->message, msg);
        c->timestamp = timestamp;
        c->parent = last;
        last = c;
    }

    fclose(f);
    commitHead = last;
}

// Save branches to file
void saveBranchesToFile() {
    FILE* f = fopen(".minigit/branches.txt", "w");
    if (!f) return;

    Branch* temp = branchHead;
    while (temp) {
        fprintf(f, "%s %lu\n", temp->name,
                (temp->head ? temp->head->commitId : 0));
        temp = temp->next;
    }

    fclose(f);
}


// Load branches from file
void loadBranchesFromFile() {
    branchHead = NULL;

    FILE* f = fopen(".minigit/branches.txt", "r");
    if (!f) return;

    char name[100];
    unsigned long commitId;

    while (fscanf(f, "%s %lu", name, &commitId) == 2) {
        Branch* b = (Branch*)malloc(sizeof(Branch));
        strcpy(b->name, name);

        // Try to find matching commit by ID
        Commit* c = commitHead;
        while (c && c->commitId != commitId) {
            c = c->parent;
        }

        b->head = c;
        b->next = branchHead;
        branchHead = b;
    }

    fclose(f);
}

// Save current branch name
void saveHEAD(const char* branchName) {
    FILE* f = fopen(".minigit/HEAD.txt", "w");
    if (!f) return;
    fprintf(f, "%s\n", branchName);
    fclose(f);
}

// Load current branch name
void loadHEAD(char* currentBranch) {
    FILE* f = fopen(".minigit/HEAD.txt", "r");
    if (!f) {
        strcpy(currentBranch, "main");
        return;
    }
    fscanf(f, "%s", currentBranch);
    fclose(f);
}

// Create a new branch from current commit
void createBranch(char* name) {
    restoreLastCommit();
    loadBranchesFromFile();

    if (commitHead == NULL) {
        printf("No commits yet. Cannot create branch.\n");
        return;
    }

    Branch* newBranch = (Branch*)malloc(sizeof(Branch));
    strcpy(newBranch->name, name);
    newBranch->head = commitHead;
    newBranch->next = branchHead;
    branchHead = newBranch;

    saveBranchesToFile();
    saveHEAD(name);

    printf("Branch '%s' created at commit %lu\n", name, commitHead->commitId);
    printf("Switched to branch '%s'\n", name); // auto-switch after creation
}

// List all branches
void listBranches() {
    restoreLastCommit();
    loadBranchesFromFile();

    char currentBranch[100];
    loadHEAD(currentBranch);

    if (branchHead == NULL) {
        printf("No branches created yet.\n");
        return;
    }

    printf("Branches:\n");
    Branch* temp = branchHead;
    while (temp != NULL) {
        if (strcmp(temp->name, currentBranch) == 0)
            printf("* %s (HEAD)\n", temp->name);   // ✅ just branch name
        else
            printf("  %s\n", temp->name);
        temp = temp->next;
    }
}
