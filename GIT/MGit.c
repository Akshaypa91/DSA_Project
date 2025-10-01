#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MINI_GIT_DIR ".mgit"
#define OBJECTS_DIR MINI_GIT_DIR "/objects"
#define COMMITS_DIR MINI_GIT_DIR "/commits"
#define HEAD_FILE MINI_GIT_DIR "/HEAD.txt"
#define INDEX_FILE MINI_GIT_DIR "/index.txt"
#define BRANCHES_FILE MINI_GIT_DIR "/branches.txt"

#define MAX_LINE 1024
#define MAX_FILES 1024

// Simple djb2 hash function (like your custom hash)
char* Hash(const char* content) {
    static char hashStr[64];
    unsigned long hash = 5381;
    int c;
    while ((c = *content++)) {
        hash = ((hash << 5) + hash) + c;
    }
    snprintf(hashStr, sizeof(hashStr), "%lu", hash);
    return hashStr;
}

// Get current time string
char* getCurrentTime() {
    time_t now = time(NULL);
    return ctime(&now);
}

// Check if directory exists
int dir_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Create directory
void create_dir(const char* path) {
    if (mkdir(path, 0777) != 0) {
        perror("mkdir failed");
    }
}

// Initialize mgit
void init() {
    if (dir_exists(MINI_GIT_DIR)) {
        printf("mgit is already initialized.\n");
        return;
    }

    create_dir(MINI_GIT_DIR);
    create_dir(OBJECTS_DIR);
    create_dir(COMMITS_DIR);

    FILE* f = fopen(HEAD_FILE, "w");
    if (f) { fprintf(f, "main"); fclose(f); }

    f = fopen(BRANCHES_FILE, "w");
    if (f) { fprintf(f, "main:null\n"); fclose(f); }

    f = fopen(INDEX_FILE, "w"); if(f) fclose(f);

    printf("Initialized empty mgit repository.\n");
}

// Add file to staging
void add(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) { printf("File not found: %s\n", filename); return; }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* content = (char*)malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);

    char* hash = Hash(content);

    char outpath[256];
    snprintf(outpath, sizeof(outpath), "%s/%s", OBJECTS_DIR, hash);
    f = fopen(outpath, "w");
    if (f) { fwrite(content, 1, size, f); fclose(f); }

    f = fopen(INDEX_FILE, "a");
    if (f) { fprintf(f, "%s:%s\n", filename, hash); fclose(f); }

    printf("Staged file: %s\n", filename);
    free(content);
}

// Get current branch from HEAD
void getCurrentBranch(char* branch) {
    FILE* f = fopen(HEAD_FILE, "r");
    if (!f) { strcpy(branch, "main"); return; }
    fgets(branch, MAX_LINE, f);
    branch[strcspn(branch, "\n")] = 0;
    fclose(f);
}

// Get branch head commit
void getBranchHead(const char* branch, char* head) {
    FILE* f = fopen(BRANCHES_FILE, "r");
    if (!f) { strcpy(head, "null"); return; }
    char line[MAX_LINE];
    while (fgets(line, MAX_LINE, f)) {
        char* sep = strchr(line, ':');
        if (!sep) continue;
        *sep = 0;
        char* name = line;
        char* commit = sep + 1;
        commit[strcspn(commit, "\n")] = 0;
        if (strcmp(name, branch) == 0) { strcpy(head, commit); fclose(f); return; }
    }
    strcpy(head, "null");
    fclose(f);
}

// Update branch head commit
void updateBranchHead(const char* branch, const char* newHash) {
    FILE* f = fopen(BRANCHES_FILE, "r");
    if (!f) return;

    char temp[1024*MAX_LINE] = {0};
    char line[MAX_LINE];
    while(fgets(line, MAX_LINE, f)) {
        char name[MAX_LINE], commit[MAX_LINE];
        sscanf(line, "%[^:]:%s", name, commit);
        if (strcmp(name, branch) == 0) {
            strcat(temp, name); strcat(temp, ":"); strcat(temp, newHash); strcat(temp, "\n");
        } else {
            strcat(temp, line);
        }
    }
    fclose(f);

    f = fopen(BRANCHES_FILE, "w");
    if(f){ fputs(temp, f); fclose(f); }
}

// Commit staged files
void commit(const char* message) {
    FILE* index = fopen(INDEX_FILE, "r");
    if (!index) { printf("Nothing to commit.\n"); return; }

    char commitData[1024*MAX_LINE] = {0};
    char line[MAX_LINE];
    while(fgets(line, MAX_LINE, index)) strcat(commitData, line);
    fclose(index);

    char branch[MAX_LINE];
    getCurrentBranch(branch);

    char parent[MAX_LINE];
    getBranchHead(branch, parent);

    char metadata[4096];
snprintf(metadata, sizeof(metadata),
         "message: %s\ntimestamp: %sparent: %s\nbranch: %s\n",
         message, getCurrentTime(), parent, branch);


    char* hash = Hash(strcat(metadata, commitData));

    char commitPath[256];
    snprintf(commitPath, sizeof(commitPath), "%s/%s", COMMITS_DIR, hash);
    FILE* f = fopen(commitPath, "w");
    if(f){ fputs(metadata, f); fputs(commitData, f); fclose(f); }

    updateBranchHead(branch, hash);

    f = fopen(INDEX_FILE, "w"); if(f) fclose(f);

    printf("Committed. Hash: %s\n", hash);
}

// Main dispatcher
int main(int argc, char* argv[]) {
    if(argc<2){
        printf("Usage: ./mgit <command>\n");
        return 1;
    }

    if(strcmp(argv[1], "init") == 0) init();
    else if(strcmp(argv[1], "add") == 0 && argc>=3) add(argv[2]);
    else if(strcmp(argv[1], "commit") == 0 && argc>=4 && strcmp(argv[2], "-m")==0) commit(argv[3]);
    else printf("Unknown command.\n");

    return 0;
}