// branch.c //testing3.
#include "utils.h"

// Restore last commit if memory is empty
void restoreLastCommit() {
    if (commitHead != NULL) return;

    FILE* f = fopen(".minigit/commits.txt", "r");
    if (!f) return;

    char id[64], msg[256];
    time_t timestamp;
    Commit* last = NULL;

    while (fscanf(f, "%s : %[^:] : %lld\n", id, msg, (long long *)&timestamp) == 3)
 {
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

// Helper: check if a branch already exists in memory
static int branchExists(const char* name) {
    Branch* t = branchHead;
    while (t) {
        if (strcmp(t->name, name) == 0) return 1;
        t = t->next;
    }
    return 0;
}

// Save all branches to file
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

// Load branches (skips duplicates automatically)
void loadBranchesFromFile() {
    branchHead = NULL;

    FILE* f = fopen(".minigit/branches.txt", "r");
    if (!f) return;

    char name[100];
    unsigned long commitId;

    while (fscanf(f, "%s %lu", name, &commitId) == 2) {
        // avoid duplicate names already in memory
        if (branchExists(name)) continue;

        Branch* b = (Branch*)malloc(sizeof(Branch));
        strcpy(b->name, name);

        // try to match commit by ID
        Commit* c = commitHead;
        while (c && c->commitId != commitId)
            c = c->parent;

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

// Create a new branch (no duplicates)
void createBranch(char* name) {
    restoreLastCommit();
    loadBranchesFromFile();

    if (commitHead == NULL) {
        printf("No commits yet. Cannot create branch.\n");
        return;
    }

    if (branchExists(name)) {
        printf("Branch '%s' already exists.\n", name);
        saveHEAD(name);
        printf("Switched to branch '%s'\n", name);
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
    printf("Switched to branch '%s'\n", name);
}

// List all branches (no commit IDs shown)
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
            printf("* %s (HEAD)\n", temp->name);
        else
            printf("  %s\n", temp->name);
        temp = temp->next;
    }
}
