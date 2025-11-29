// diff.c
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_LINES 1000
#define MAX_LEN   1024   // increased to allow longer lines

// Utility:- Split file into lines
// Returns number of lines read (>=0), or -1 on error (file open failed).
int readFileLines(const char* filename, char lines[MAX_LINES][MAX_LEN]) {
	FILE* f = fopen(filename, "r");
	if (!f) return -1; // distinguish error from zero lines

	int count = 0;
	while (fgets(lines[count], MAX_LEN, f)) {
		// remove newline(s)
		lines[count][strcspn(lines[count], "\r\n")] = '\0';
		count++;
		if (count >= MAX_LINES) break;
	}
	fclose(f);
	return count;
}

// Helper to allocate DP table of size (n+1)*(m+1)
// returns pointer to allocated int array or NULL on failure.
// Access element (i,j) as dp[i*(m+1) + j]
int *allocDP(int n, int m) {
	int rows = n + 1;
	int cols = m + 1;
	// guard against absurd sizes
	if (rows <= 0 || cols <= 0) return NULL;
	// try to allocate contiguous block
	int *dp = (int *)calloc((size_t)rows * cols, sizeof(int));
	return dp;
}

// LCS Table: returns length of LCS, fills dp (contiguous array of size (n+1)*(m+1))
// dp must be allocated with allocDP(n,m)
int LCS(char A[MAX_LINES][MAX_LEN], int n,
        char B[MAX_LINES][MAX_LEN], int m,
        int *dp) {
	if (!dp) return 0;
	int cols = m + 1;
	for (int i = 0; i <= n; i++) {
		for (int j = 0; j <= m; j++) {
			if (i == 0 || j == 0) dp[i*cols + j] = 0;
			else if (strcmp(A[i-1], B[j-1]) == 0)
				dp[i*cols + j] = 1 + dp[(i-1)*cols + (j-1)];
			else {
				int up = dp[(i-1)*cols + j];
				int left = dp[i*cols + (j-1)];
				dp[i*cols + j] = (up > left) ? up : left;
			}
		}
	}
	return dp[n*cols + m];
}

// Print Diff recursively using the dp table.
// dp is contiguous with cols = m+1
void printDiffRecursive(char A[MAX_LINES][MAX_LEN], int n,
                        char B[MAX_LINES][MAX_LEN], int m,
                        int *dp) {
	if (n == 0 && m == 0) return;

	int cols = m + 1;

	if (n > 0 && m > 0 && strcmp(A[n-1], B[m-1]) == 0) {
		printDiffRecursive(A, n-1, B, m-1, dp);
		printf("   %s\n", A[n-1]); // no change
	}
	else if (m > 0 && (n == 0 || dp[n*cols + (m-1)] >= dp[(n-1)*cols + m])) {
		printDiffRecursive(A, n, B, m-1, dp);
		printf("+  %s\n", B[m-1]); // added
	}
	else if (n > 0 && (m == 0 || dp[n*cols + (m-1)] < dp[(n-1)*cols + m])) {
		printDiffRecursive(A, n-1, B, m, dp);
		printf("-  %s\n", A[n-1]); // removed
	}
}

// Diff two files
void diffFiles(const char* file1, const char* file2) {
	char A[MAX_LINES][MAX_LEN], B[MAX_LINES][MAX_LEN];
	int n = readFileLines(file1, A);
	int m = readFileLines(file2, B);

	if (n < 0) {
		printf("Error: cannot open %s\n", file1);
		return;
	}
	if (m < 0) {
		printf("Error: cannot open %s\n", file2);
		return;
	}

	if (n == 0 && m == 0) {
		printf("Both files are empty.\n");
		return;
	}

	// allocate dp dynamically to avoid large stack usage
	int *dp = allocDP(n, m);
	if (!dp) {
		printf("Error: failed to allocate memory for diff.\n");
		return;
	}

	LCS(A, n, B, m, dp);

	printf("Diff between %s and %s:\n", file1, file2);
	printSeparator();
	printDiffRecursive(A, n, B, m, dp);
	printSeparator();

	free(dp);
}

// Diff between commits
void diffCommits(unsigned long c1, unsigned long c2) {
	char path1[256], path2[256];

	snprintf(path1, sizeof(path1), ".minigit/commits/%lu.txt", c1);
	snprintf(path2, sizeof(path2), ".minigit/commits/%lu.txt", c2);

	if (!fileExists(path1) || !fileExists(path2)) {
		printf("One or both commit files not found: %s , %s\n", path1, path2);
		return;
	}

	// reuse diffFiles which already handles reading lines and printing diff
	diffFiles(path1, path2);
}