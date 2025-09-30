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
        printf("If mgit is not added to system path you can use the commands listed below as they are.\n");
        printf("But if mgit is added to system path you can remove the ./ before mgit to use it.\n\n");
        printf("1. ./mgit init\n");
        printf("2. ./mgit add <file>\n");
        return 1;
    }

    if (strcmp(argv[1], "init") == 0) {
        init();
    }
    else if (strcmp(argv[1], "add") == 0 && argc >= 3) {
        add(argv[2]);
    }
    else {
        printf("Unknown or incomplete command.\n");
    }

    return 0;
}