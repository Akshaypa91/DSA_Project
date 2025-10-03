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
void diffCommits(const char* commit1, const char* commit2, const char* filename) {
	char path1[256], path2[256];
	snprintf(path1, sizeof(path1), ".minigit/commits/%s/%s", commit1, filename);
	snprintf(path2, sizeof(path2), ".minigit/commits/%s/%s", commit2, filename);

	if (!fileExists(path1) || !fileExists(path2)) {
		printf("File %s not found in one of the commits!\n", filename);
		return;
	}
	diffFiles(path1, path2);
}