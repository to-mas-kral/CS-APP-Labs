/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include "cachelab.h"
#include <stdio.h>

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if (M == 32 && N == 32) {
        for (int r = 0; r < N; r += 8) { // matrix row
            for (int c = 0; c < M; c += 8) { // matrix column
                for (int br = 0; br < 8; br++) { // block row
                    int _0, _1, _2, _3, _4, _5, _6, _7;
                    _0 = A[r + br][c + 0];
                    _1 = A[r + br][c + 1];
                    _2 = A[r + br][c + 2];
                    _3 = A[r + br][c + 3];
                    _4 = A[r + br][c + 4];
                    _5 = A[r + br][c + 5];
                    _6 = A[r + br][c + 6];
                    _7 = A[r + br][c + 7];

                    B[c + 0][r + br] = _0;
                    B[c + 1][r + br] = _1;
                    B[c + 2][r + br] = _2;
                    B[c + 3][r + br] = _3;
                    B[c + 4][r + br] = _4;
                    B[c + 5][r + br] = _5;
                    B[c + 6][r + br] = _6;
                    B[c + 7][r + br] = _7;
                }
            }
        }
    }

    // My not optimal solution
    if (M == 64 && N == 64) {
        for (int r = 0; r < N; r += 8) { // matrix row
            for (int c = 0; c < M; c += 8) { // matrix column
                int _10, _11, _12, _13, _20, _21, _22, _23;
                for (int ch = 0; ch < 8; ch += 2) { // 2x4 chunk index
                    _10 = A[r + ch][c + 0];
                    _11 = A[r + ch][c + 1];
                    _12 = A[r + ch][c + 2];
                    _13 = A[r + ch][c + 3];
                    _20 = A[r + ch + 1][c + 0];
                    _21 = A[r + ch + 1][c + 1];
                    _22 = A[r + ch + 1][c + 2];
                    _23 = A[r + ch + 1][c + 3];

                    B[c + 0][r + ch] = _10;
                    B[c + 1][r + ch] = _11;
                    B[c + 2][r + ch] = _12;
                    B[c + 3][r + ch] = _13;
                    B[c + 0][r + ch + 1] = _20;
                    B[c + 1][r + ch + 1] = _21;
                    B[c + 2][r + ch + 1] = _22;
                    B[c + 3][r + ch + 1] = _23;
                }

                c += 4; // Switch to next column of 2x4 ints, we can't introduce another declaration here
                    //as per rules of the assignment

                for (int ch = 0; ch < 8; ch += 2) { // 2x4 chunk index
                    _10 = A[r + ch][c + 0];
                    _11 = A[r + ch][c + 1];
                    _12 = A[r + ch][c + 2];
                    _13 = A[r + ch][c + 3];
                    _20 = A[r + ch + 1][c + 0];
                    _21 = A[r + ch + 1][c + 1];
                    _22 = A[r + ch + 1][c + 2];
                    _23 = A[r + ch + 1][c + 3];

                    B[c + 0][r + ch] = _10;
                    B[c + 1][r + ch] = _11;
                    B[c + 2][r + ch] = _12;
                    B[c + 3][r + ch] = _13;
                    B[c + 0][r + ch + 1] = _20;
                    B[c + 1][r + ch + 1] = _21;
                    B[c + 2][r + ch + 1] = _22;
                    B[c + 3][r + ch + 1] = _23;
                }

                c -= 4;
            }
        }
    }

    // Better solution that I found on the internet
    /* if (M == 64 && N == 64) {
        for (int c = 0; c < M; c += 8) { // matrix column
            for (int r = 0; r < N; r += 8) { // matrix row
                int _0, _1, _2, _3, _4, _5, _6, _7;
                int br; // block row
                for (br = 0; br < 4; br++) {
                    _0 = A[r + br][c + 0];
                    _1 = A[r + br][c + 1];
                    _2 = A[r + br][c + 2];
                    _3 = A[r + br][c + 3];
                    _4 = A[r + br][c + 4];
                    _5 = A[r + br][c + 5];
                    _6 = A[r + br][c + 6];
                    _7 = A[r + br][c + 7];

                    B[c + 0][r + br] = _0;
                    B[c + 1][r + br] = _1;
                    B[c + 2][r + br] = _2;
                    B[c + 3][r + br] = _3;
                    B[c + 0][r + br + 4] = _4;
                    B[c + 1][r + br + 4] = _5;
                    B[c + 2][r + br + 4] = _6;
                    B[c + 3][r + br + 4] = _7;
                }

                for (br = 0; br < 4; br++) {
                    _0 = B[c + br][r + 4 + 0];
                    _1 = B[c + br][r + 4 + 1];
                    _2 = B[c + br][r + 4 + 2];
                    _3 = B[c + br][r + 4 + 3];

                    B[c + br][r + 4 + 0] = A[r + 4 + 0][c + br];
                    B[c + br][r + 4 + 1] = A[r + 4 + 1][c + br];
                    B[c + br][r + 4 + 2] = A[r + 4 + 2][c + br];
                    B[c + br][r + 4 + 3] = A[r + 4 + 3][c + br];

                    B[c + 4 + br][r + 0] = _0;
                    B[c + 4 + br][r + 1] = _1;
                    B[c + 4 + br][r + 2] = _2;
                    B[c + 4 + br][r + 3] = _3;

                    B[c + 4 + br][r + 4] = A[r + 4 + 0][c + 4 + br];
                    B[c + 4 + br][r + 5] = A[r + 4 + 1][c + 4 + br];
                    B[c + 4 + br][r + 6] = A[r + 4 + 2][c + 4 + br];
                    B[c + 4 + br][r + 7] = A[r + 4 + 3][c + 4 + br];
                }
            }
        }*/

    if (M == 61 && N == 67) {
        int BS = 8; // Block size
        for (int r = 0; r < N; r += BS) { // matrix row
            for (int c = 0; c < M; c += BS) { // matrix column
                for (int bc = 0; bc < BS && (c + bc < 61); bc++) { // block column
                    for (int br = 0; (br < BS) && r + br < 67; br++) { // block row
                        B[c + bc][r + br] = A[r + br][c + bc];
                    }
                }
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
