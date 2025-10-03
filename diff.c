// diff.c
#include "utils.h"

#define MAX_LINES 1000
#define MAX_LEN   256

// ==== Utility: Split file into lines ==== //
int readFileLines(const char* filename, char lines[MAX_LINES][MAX_LEN]) {
	FILE* f = fopen(filename, "r");
	if (!f) return 0;

	int count = 0;
	while (fgets(lines[count], MAX_LEN, f)) {
		// remove newline
		lines[count][strcspn(lines[count], "\n")] = '\0';
		count++;
		if (count >= MAX_LINES) break;
	}
	fclose(f);
	return count;
}

// ==== LCS Table ==== //
int LCS(char A[MAX_LINES][MAX_LEN], int n,
        char B[MAX_LINES][MAX_LEN], int m,
        int dp[MAX_LINES+1][MAX_LINES+1]) {
	for (int i = 0; i <= n; i++) {
		for (int j = 0; j <= m; j++) {
			if (i == 0 || j == 0) dp[i][j] = 0;
			else if (strcmp(A[i-1], B[j-1]) == 0)
				dp[i][j] = 1 + dp[i-1][j-1];
			else
				dp[i][j] = (dp[i-1][j] > dp[i][j-1]) ? dp[i-1][j] : dp[i][j-1];
		}
	}
	return dp[n][m];
}

// ==== Print Diff ==== //
void printDiffRecursive(char A[MAX_LINES][MAX_LEN], int n,
                        char B[MAX_LINES][MAX_LEN], int m,
                        int dp[MAX_LINES+1][MAX_LINES+1]) {
	if (n > 0 && m > 0 && strcmp(A[n-1], B[m-1]) == 0) {
		printDiffRecursive(A, n-1, B, m-1, dp);
		printf("   %s\n", A[n-1]); // no change
	}
	else if (m > 0 && (n == 0 || dp[n][m-1] >= dp[n-1][m])) {
		printDiffRecursive(A, n, B, m-1, dp);
		printf("+  %s\n", B[m-1]); // added
	}
	else if (n > 0 && (m == 0 || dp[n][m-1] < dp[n-1][m])) {
		printDiffRecursive(A, n-1, B, m, dp);
		printf("-  %s\n", A[n-1]); // removed
	}
}

// ==== Diff two files ==== //
void diffFiles(const char* file1, const char* file2) {
	char A[MAX_LINES][MAX_LEN], B[MAX_LINES][MAX_LEN];
	int n = readFileLines(file1, A);
	int m = readFileLines(file2, B);

	if (n == 0 && m == 0) {
		printf("Both files are empty or not found!\n");
		return;
	}

	int dp[MAX_LINES+1][MAX_LINES+1];
	LCS(A, n, B, m, dp);

	printf("Diff between %s and %s:\n", file1, file2);
	printSeparator();
	printDiffRecursive(A, n, B, m, dp);
	printSeparator();
}

// ==== Diff between commits (naive: assumes same filenames) ==== //
// Simple diff: compare two commit snapshot files
void diffCommits(unsigned long c1, unsigned long c2) {
	char path1[256], path2[256];

	snprintf(path1, sizeof(path1), ".minigit/commits/%lu.txt", c1);
	snprintf(path2, sizeof(path2), ".minigit/commits/%lu.txt", c2);

	if (!fileExists(path1) || !fileExists(path2)) {
		printf("One or both commit files not found!\n");
		return;
	}

	FILE *f1 = fopen(path1, "r");
	FILE *f2 = fopen(path2, "r");
	if (!f1 || !f2) {
		printf("Error opening commit files!\n");
		return;
	}

	printf("Diff between commits %lu and %lu:\n", c1, c2);

	char line1[256], line2[256];
	while (fgets(line1, sizeof(line1), f1) && fgets(line2, sizeof(line2), f2)) {
		if (strcmp(line1, line2) != 0) {
			printf("- %s+ %s\n", line1, line2);
		}
	}

	fclose(f1);
	fclose(f2);
}