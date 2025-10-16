#include "utils.h"
#include <time.h>

void logCommits() {
	FILE* f = fopen(".minigit/commits.txt", "r");
	if (!f) {
		printf("No commits yet.\n");
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), f)) {
		char id[32], msg[256];
		long ts;
		if (sscanf(line, "%s : %[^:]: %ld", id, msg, &ts) == 3) {
			printf("Commit ID: %s\n", id);
			printf("Message: %s\n", msg);

			time_t t = (time_t)ts;
			char* tsStr = ctime(&t);
			tsStr[strcspn(tsStr, "\n")] = 0; //remove newline
			printf("Timestamp: %s\n", tsStr);
			printf("-----------------------------\n");
		}
	}
	fclose(f);
}