#include <stdio.h>
#include <string.h>

// Declarations for functions implemented elsewhere
void init();
void add(const char* filename);
void commit(const char* message);
void log_history();     // renamed from "log" since log() conflicts with math.h
void createBranch(const char* branch);
void checkout(const char* branch);
void merge(const char* branch);
void diff(const char* commit1, const char* commit2);

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("If minigit is not added to system path you can use the commands listed below as they are.\n");
        printf("But if minigit is added to system path you can remove the ./ before minigit to use it.\n\n");
        printf("1. ./minigit init\n");
        printf("2. ./minigit add <file>\n");
        printf("3. ./minigit commit -m 'commit message goes here'\n");
        printf("4. ./minigit log\n");
        printf("5. ./minigit branch branch_name\n");
        printf("6. ./minigit checkout branch_name\n");
        printf("7. ./minigit merge branch_name  (But don't forget to checkout to the specific branch first.)\n");
        printf("8. ./minigit diff <commit1> <commit2>  (You can get the commit hash of two files using log.)\n\n");
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        init();
    }
    else if (strcmp(argv[1], "add") == 0 && argc >= 3) {
        add(argv[2]);
    }
    else if (strcmp(argv[1], "commit") == 0 && argc >= 4 && strcmp(argv[2], "-m") == 0) {
        commit(argv[3]);
    }
    else if (strcmp(argv[1], "log") == 0) {
        log_history();
    }
    else if (strcmp(argv[1], "branch") == 0 && argc >= 3) {
        createBranch(argv[2]);
    }
    else if (strcmp(argv[1], "checkout") == 0 && argc >= 3) {
        checkout(argv[2]);
    }
    else if (strcmp(argv[1], "merge") == 0 && argc >= 3) {
        merge(argv[2]);
    }
    else if (strcmp(argv[1], "diff") == 0 && argc >= 4) {
        diff(argv[2], argv[3]);
    }
    else {
        printf("Unknown or incomplete command.\n");
    }

    return 0;
}