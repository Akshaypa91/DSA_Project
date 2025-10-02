// utils.c
#include "utils.h"

// ====================== COMMON UTILS ====================== //

// Simple polynomial rolling hash for strings (used in commit IDs)
unsigned long simpleHash(char* str) {
	unsigned long hash = 0;
	int p = 31;  // prime base
	for (int i = 0; str[i]; i++) {
		hash = hash * p + str[i];
	}
	return hash;
}

// Safely create a directory (ignore if exists)
void makeDir(const char* folder) {
	#ifdef _WIN32
		mkdir(folder);
	#else
		mkdir(folder, 0700);
	#endif
}

// Read entire file into buffer (for diff, commit snapshot etc.)
char* readFile(const char* filename) {
	FILE* f = fopen(filename, "r");
	if (!f) return NULL;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);

	char* buffer = (char*)malloc(size + 1);
	if (!buffer) { fclose(f); return NULL; }

	fread(buffer, 1, size, f);
	buffer[size] = '\0';
	fclose(f);
	return buffer;
}

// Write content to file (overwrite mode)
void writeFile(const char* filename, const char* content) {
	FILE* f = fopen(filename, "w");
	if (!f) return;
	fprintf(f, "%s", content);
	fclose(f);
}

// Print horizontal separator (for logs/commits)
void printSeparator() {
	printf("=====================================\n");
}