/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

#define min(x, y) ((x) < (y) ? (x) : (y))

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

void transpose_8_8(int *A, int *B, int S, int BS);
void transpose_32_32(int *A, int *B, int S, int BS);
void transpose_32_32_0(int *A, int *B, int S, int BS);
void transpose_32_32_1(int *A, int *B, int S, int BS);
void transpose_32_32_2(int *A, int *B, int S, int BS);
void transpose_64_64(int *A, int *B, int S, int BS);
void transpose_64_64_0(int *A, int *B, int S, int BS);
void transpose_64_64_1(int *A, int *B, int S, int BS);
void transpose_64_64_2(int *A, int *B, int S, int BS);
void transpose_64_64_3(int *A, int *B, int S, int BS);
void transpose_64_64_4(int *A, int *B, int S, int BS);
void transpose_64_64_5(int *A, int *B, int S, int BS);
void transpose_64_64_6(int *A, int *B, int S, int BS);
void transpose_61_67(int M, int N, int A[N][M], int B[M][N]);

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
  if(M == 32 && N == 32)
  {
    transpose_32_32_2(A[0], B[0], 32, 8);
  }
  else if(M == 64 && N == 64)
    transpose_64_64_5(A[0], B[0], 64, 4);
  else if(M == 61 && N == 67)
  {
    const int Si = 17, Sj = 4;
    for(int ii = 0; ii < N; ii += Si)
      for(int jj = 0; jj < M; jj += Sj)
        for(int i = ii; i < min(ii + Si, N); ++i)
          for(int j = jj; j < min(jj + Sj, M); ++j)
            B[j][i] = A[i][j];
  }
  else
  {
    void trans(int, int, int [N][M], int [M][N]);
    trans(M, N, A, B);
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

/* BS=8, 344 misses */
void transpose_32_32_0(int *A, int *B, int S, int BS)
{
  const int MN = 32;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
      for(int i = ii; i < ii + BS; ++i)
        for(int j = jj; j < jj + BS; ++j)
          // B[j][i] = A[i][j];
          B[j * S + i] = A[i * S + j];
}

/* BS=8, 288 misses */
void transpose_32_32_1(int *A, int *B, int S, int BS)
{
  BS = 8;
  const int MN = 32;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
      for(int i = ii; i < ii + BS; ++i)
      {
        int b0, b1, b2, b3, b4, b5, b6, b7;
        b0 = A[i * S + (jj + 0)];
        b1 = A[i * S + (jj + 1)];
        b2 = A[i * S + (jj + 2)];
        b3 = A[i * S + (jj + 3)];
        b4 = A[i * S + (jj + 4)];
        b5 = A[i * S + (jj + 5)];
        b6 = A[i * S + (jj + 6)];
        b7 = A[i * S + (jj + 7)];
        B[(jj + 0) * S + i] = b0;
        B[(jj + 1) * S + i] = b1;
        B[(jj + 2) * S + i] = b2;
        B[(jj + 3) * S + i] = b3;
        B[(jj + 4) * S + i] = b4;
        B[(jj + 5) * S + i] = b5;
        B[(jj + 6) * S + i] = b6;
        B[(jj + 7) * S + i] = b7;
      }
}

/* BS=8, 260 misses (4 irrelevant), best optimized */
void transpose_32_32_2(int *A, int *B, int S, int BS)
{
  BS = 8;
  const int MN = 32;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
    {
      for(int i = 0; i < BS; ++i)
        // for(int j = 0; j < BS; ++j)
        //   B[(i + jj) * S + (j + ii)] = A[(i + ii) * S + (j + jj)];
      {
        int b0, b1, b2, b3, b4, b5, b6, b7;
        b0 = A[(i + ii) * S + (0 + jj)];
        b1 = A[(i + ii) * S + (1 + jj)];
        b2 = A[(i + ii) * S + (2 + jj)];
        b3 = A[(i + ii) * S + (3 + jj)];
        b4 = A[(i + ii) * S + (4 + jj)];
        b5 = A[(i + ii) * S + (5 + jj)];
        b6 = A[(i + ii) * S + (6 + jj)];
        b7 = A[(i + ii) * S + (7 + jj)];
        B[(i + jj) * S + (0 + ii)] = b0;
        B[(i + jj) * S + (1 + ii)] = b1;
        B[(i + jj) * S + (2 + ii)] = b2;
        B[(i + jj) * S + (3 + ii)] = b3;
        B[(i + jj) * S + (4 + ii)] = b4;
        B[(i + jj) * S + (5 + ii)] = b5;
        B[(i + jj) * S + (6 + ii)] = b6;
        B[(i + jj) * S + (7 + ii)] = b7;
      }
      for(int i = 0; i < BS; ++i)
        for(int j = i + 1; j < BS; ++j)
        {
          int Bijjjii = B[(i + jj) * S + (j + ii)];
          int Bjjjiii = B[(j + jj) * S + (i + ii)];
          B[(j + jj) * S + (i + ii)] = Bijjjii;
          B[(i + jj) * S + (j + ii)] = Bjjjiii;
        }
    }
}

/* BS=8, 276 misses */
void transpose_32_32(int *A, int *B, int S, int BS)
{
  const int MN = 32;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
      if(ii != jj)
        for(int i = ii; i < ii + BS; ++i)
          for(int j = jj; j < jj + BS; ++j)
            // B[j][i] = A[i][j];
            B[j * S + i] = A[i * S + j];
  for(int ii = 0; ii < MN; ii += BS)
    for(int i = ii; i < ii + BS; ++i)
      for(int j = ii; j < ii + BS; ++j)
        // B[MN - 1 - j][MN - 1 - i] = A[i][j];
        B[(MN - 1 - j) * S + (MN - 1 - i)] = A[i * S + j];
  for(int ii = 0; ii < MN / 2; ii += BS)
    for(int i = ii; i < ii + BS; ++i)
      for(int j = ii; j < ii + BS; ++j)
      {
        // int tmp = B[i][j];
        int tmp = B[i * S + j];
        // B[i][j] = B[MN - 1 - i][MN - 1 - j];
        B[i * S + j] = B[(MN - 1 - i) * S + (MN - 1 - j)];
        // B[MN - 1 - i][MN - 1 - j] = tmp;
        B[(MN - 1 - i) * S + (MN - 1 - j)] = tmp;
      }
}

/* BS=4, 1796 misses */
void transpose_64_64(int *A, int *B, int S, int BS)
{
  const int MN = 64;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
      if(ii != jj)
        for(int i = ii; i < ii + BS; ++i)
          for(int j = jj; j < jj + BS; ++j)
            // B[j][i] = A[i][j];
            B[j * S + i] = A[i * S + j];
  for(int ii = 0; ii < MN; ii += BS)
    for(int i = ii; i < ii + BS; ++i)
      for(int j = ii; j < ii + BS; ++j)
        // B[MN - 1 - j][MN - 1 - i] = A[i][j];
        B[(MN - 1 - j) * S + (MN - 1 - i)] = A[i * S + j];
  for(int ii = 0; ii < MN / 2; ii += BS)
    for(int i = ii; i < ii + BS; ++i)
      for(int j = ii; j < ii + BS; ++j)
      {
        // int tmp = B[i][j];
        int tmp = B[i * S + j];
        // B[i][j] = B[MN - 1 - i][MN - 1 - j];
        B[i * S + j] = B[(MN - 1 - i) * S + (MN - 1 - j)];
        // B[MN - 1 - i][MN - 1 - j] = tmp;
        B[(MN - 1 - i) * S + (MN - 1 - j)] = tmp;
      }
}

/* BS=4, 1908 misses */
void transpose_64_64_0(int *A, int *B, int S, int BS)
{
  transpose_32_32(&A[0], &B[0], S, BS);
  transpose_32_32(&A[32], &B[32 * S], S, BS);
  transpose_32_32(&A[32 * S], &B[32], S, BS);
  transpose_32_32(&A[32 * S + 32], &B[32 * S + 32], S, BS);
}

/* BS=4, 2020 misses */
void transpose_64_64_1(int *A, int *B, int S, int BS)
{
  transpose_32_32(&A[32], &B[32 * S], S, BS);
  transpose_32_32(&A[32 * S], &B[32], S, BS);
  transpose_32_32(&A[0], &B[32 * S + 32], S, BS);
  transpose_32_32(&A[32 * S + 32], &B[0], S, BS);
  for(int i = 0; i < 32; ++i)
    for(int j = 0; j < 32; ++j)
    {
      int tmp = B[i * S + j];
      B[i * S + j] = B[(32 + i) * S + (32 + j)];
      B[(32 + i) * S + (32 + j)] = tmp;
    }
}

/* BS=4, 1700 misses */
void transpose_64_64_2(int *A, int *B, int S, int BS)
{
  for(int i = 0; i < 64; i += 8)
    for(int j = 0; j < 64; j += 8)
      transpose_8_8(&A[i * S + j], &B[j * S + i], S, BS);
}

/* BS=4, 1700 misses */
void transpose_64_64_3(int *A, int *B, int S, int BS)
{
  BS = 4;
  const int MN = 64;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
      for(int i = ii; i < ii + BS; ++i)
      {
        int b0, b1, b2, b3;
        b0 = A[i * S + (jj + 0)];
        b1 = A[i * S + (jj + 1)];
        b2 = A[i * S + (jj + 2)];
        b3 = A[i * S + (jj + 3)];
        B[(jj + 0) * S + i] = b0;
        B[(jj + 1) * S + i] = b1;
        B[(jj + 2) * S + i] = b2;
        B[(jj + 3) * S + i] = b3;
      }
}

/* BS=4, 1604 misses */
void transpose_64_64_4(int *A, int *B, int S, int BS)
{
  BS = 4;
  const int MN = 64;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
    {
      for(int i = 0; i < BS; ++i)
        // for(int j = 0; j < BS; ++j)
        //   B[(i + jj) * S + (j + ii)] = A[(i + ii) * S + (j + jj)];
      {
        int b0, b1, b2, b3;
        b0 = A[(i + ii) * S + (0 + jj)];
        b1 = A[(i + ii) * S + (1 + jj)];
        b2 = A[(i + ii) * S + (2 + jj)];
        b3 = A[(i + ii) * S + (3 + jj)];
        B[(i + jj) * S + (0 + ii)] = b0;
        B[(i + jj) * S + (1 + ii)] = b1;
        B[(i + jj) * S + (2 + ii)] = b2;
        B[(i + jj) * S + (3 + ii)] = b3;
      }
      for(int i = 0; i < BS; ++i)
        for(int j = i + 1; j < BS; ++j)
        {
          int Bijjjii = B[(i + jj) * S + (j + ii)];
          int Bjjjiii = B[(j + jj) * S + (i + ii)];
          B[(j + jj) * S + (i + ii)] = Bijjjii;
          B[(i + jj) * S + (j + ii)] = Bjjjiii;
        }
    }
}

/* BS=8--4, 1316 misses */
void transpose_64_64_5(int *A, int *B, int S, int BS)
{
  BS = 8;
  const int MN = 64;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
    {
      for(int i = 0; i < BS / 2; ++i)
      {
        int b0, b1, b2, b3, b4, b5, b6, b7, t;
        b0 = A[(i + ii) * S + (0 + jj)];
        b1 = A[(i + ii) * S + (1 + jj)];
        b2 = A[(i + ii) * S + (2 + jj)];
        b3 = A[(i + ii) * S + (3 + jj)];
        b4 = A[(i + ii) * S + (4 + jj)];
        b5 = A[(i + ii) * S + (5 + jj)];
        b6 = A[(i + ii) * S + (6 + jj)];
        b7 = A[(i + ii) * S + (7 + jj)];
        B[(i + jj) * S + (0 + ii)] = b0;
        B[(i + jj) * S + (1 + ii)] = b1;
        B[(i + jj) * S + (2 + ii)] = b2;
        B[(i + jj) * S + (3 + ii)] = b3;
        B[(i + jj) * S + (4 + ii)] = b4;
        B[(i + jj) * S + (5 + ii)] = b5;
        B[(i + jj) * S + (6 + ii)] = b6;
        B[(i + jj) * S + (7 + ii)] = b7;
        b0 = A[((i + BS / 2) + ii) * S + (0 + jj)];
        b1 = A[((i + BS / 2) + ii) * S + (1 + jj)];
        b2 = A[((i + BS / 2) + ii) * S + (2 + jj)];
        b3 = A[((i + BS / 2) + ii) * S + (3 + jj)];
        b4 = A[((i + BS / 2) + ii) * S + (4 + jj)];
        b5 = A[((i + BS / 2) + ii) * S + (5 + jj)];
        b6 = A[((i + BS / 2) + ii) * S + (6 + jj)];
        b7 = A[((i + BS / 2) + ii) * S + (7 + jj)];
        t = b0, b0 = B[(i + jj) * S + (4 + ii)], B[(i + jj) * S + (4 + ii)] = t;
        t = b1, b1 = B[(i + jj) * S + (5 + ii)], B[(i + jj) * S + (5 + ii)] = t;
        t = b2, b2 = B[(i + jj) * S + (6 + ii)], B[(i + jj) * S + (6 + ii)] = t;
        t = b3, b3 = B[(i + jj) * S + (7 + ii)], B[(i + jj) * S + (7 + ii)] = t;
        B[((i + BS / 2) + jj) * S + (0 + ii)] = b0;
        B[((i + BS / 2) + jj) * S + (1 + ii)] = b1;
        B[((i + BS / 2) + jj) * S + (2 + ii)] = b2;
        B[((i + BS / 2) + jj) * S + (3 + ii)] = b3;
        B[((i + BS / 2) + jj) * S + (4 + ii)] = b4;
        B[((i + BS / 2) + jj) * S + (5 + ii)] = b5;
        B[((i + BS / 2) + jj) * S + (6 + ii)] = b6;
        B[((i + BS / 2) + jj) * S + (7 + ii)] = b7;
      }

      for(int i = 0; i < BS / 2; ++i)
        for(int j = i + 1; j < BS / 2; ++j)
        {
          int Bijjjii = B[((i + BS / 2) + jj) * S + (j + ii)];
          int Bjjjiii = B[((j + BS / 2) + jj) * S + (i + ii)];
          B[((j + BS / 2) + jj) * S + (i + ii)] = Bijjjii;
          B[((i + BS / 2) + jj) * S + (j + ii)] = Bjjjiii;
        }
      for(int i = 0; i < BS / 2; ++i)
        for(int j = i + 1; j < BS / 2; ++j)
        {
          int Bijjjii = B[((i + BS / 2) + jj) * S + ((j + BS / 2) + ii)];
          int Bjjjiii = B[((j + BS / 2) + jj) * S + ((i + BS / 2) + ii)];
          B[((j + BS / 2) + jj) * S + ((i + BS / 2) + ii)] = Bijjjii;
          B[((i + BS / 2) + jj) * S + ((j + BS / 2) + ii)] = Bjjjiii;
        }
      for(int i = 0; i < BS / 2; ++i)
        for(int j = i + 1; j < BS / 2; ++j)
        {
          int Bijjjii = B[(i + jj) * S + (j + ii)];
          int Bjjjiii = B[(j + jj) * S + (i + ii)];
          B[(j + jj) * S + (i + ii)] = Bijjjii;
          B[(i + jj) * S + (j + ii)] = Bjjjiii;
        }
      for(int i = 0; i < BS / 2; ++i)
        for(int j = i + 1; j < BS / 2; ++j)
        {
          int Bijjjii = B[(i + jj) * S + ((j + BS / 2) + ii)];
          int Bjjjiii = B[(j + jj) * S + ((i + BS / 2) + ii)];
          B[(j + jj) * S + ((i + BS / 2) + ii)] = Bijjjii;
          B[(i + jj) * S + ((j + BS / 2) + ii)] = Bjjjiii;
        }
    }
}

/* BS=8--4, ???? misses */
void transpose_64_64_6(int *A, int *B, int S, int BS)
{
  BS = 8;
  const int MN = 64;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
    {
      if(jj % (BS * 2))
      {
        for(int i = 0; i < BS / 2; ++i)
        {
          int b0, b1, b2, b3, b4, b5, b6, b7;
          b0 = A[(i + ii) * S + (0 + jj)];
          b1 = A[(i + ii) * S + (1 + jj)];
          b2 = A[(i + ii) * S + (2 + jj)];
          b3 = A[(i + ii) * S + (3 + jj)];
          b4 = A[(i + ii) * S + (4 + jj)];
          b5 = A[(i + ii) * S + (5 + jj)];
          b6 = A[(i + ii) * S + (6 + jj)];
          b7 = A[(i + ii) * S + (7 + jj)];
          B[(0 + jj) * S + (i + ii)] = b0;
          B[(1 + jj) * S + (i + ii)] = b1;
          B[(2 + jj) * S + (i + ii)] = b2;
          B[(3 + jj) * S + (i + ii)] = b3;
          B[(0 + jj) * S + ((i + BS / 2) + ii)] = b4;
          B[(1 + jj) * S + ((i + BS / 2) + ii)] = b5;
          B[(2 + jj) * S + ((i + BS / 2) + ii)] = b6;
          B[(3 + jj) * S + ((i + BS / 2) + ii)] = b7;
        }
        for(int i = 0; i < BS / 2; ++i)
        {
          int b0, b1, b2, b3, b4, b5, b6, b7;
          b0 = A[((i + BS / 2) + ii) * S + (0 + jj)];
          b1 = A[((i + BS / 2) + ii) * S + (1 + jj)];
          b2 = A[((i + BS / 2) + ii) * S + (2 + jj)];
          b3 = A[((i + BS / 2) + ii) * S + (3 + jj)];
          b4 = A[((i + BS / 2) + ii) * S + (4 + jj)];
          b5 = A[((i + BS / 2) + ii) * S + (5 + jj)];
          b6 = A[((i + BS / 2) + ii) * S + (6 + jj)];
          b7 = A[((i + BS / 2) + ii) * S + (7 + jj)];
          B[(4 + jj) * S + (i + ii)] = b0;
          B[(5 + jj) * S + (i + ii)] = b1;
          B[(6 + jj) * S + (i + ii)] = b2;
          B[(7 + jj) * S + (i + ii)] = b3;
          B[(4 + jj) * S + ((i + BS / 2) + ii)] = b4;
          B[(5 + jj) * S + ((i + BS / 2) + ii)] = b5;
          B[(6 + jj) * S + ((i + BS / 2) + ii)] = b6;
          B[(7 + jj) * S + ((i + BS / 2) + ii)] = b7;
        }
        // ...
      }
      else
      {
        for(int i = 0; i < BS / 2; ++i)
        {
          int b0, b1, b2, b3, b4, b5, b6, b7, t;
          b0 = A[(i + ii) * S + (0 + jj)];
          b1 = A[(i + ii) * S + (1 + jj)];
          b2 = A[(i + ii) * S + (2 + jj)];
          b3 = A[(i + ii) * S + (3 + jj)];
          b4 = A[(i + ii) * S + (4 + jj)];
          b5 = A[(i + ii) * S + (5 + jj)];
          b6 = A[(i + ii) * S + (6 + jj)];
          b7 = A[(i + ii) * S + (7 + jj)];
          B[(i + jj) * S + (0 + ii)] = b0;
          B[(i + jj) * S + (1 + ii)] = b1;
          B[(i + jj) * S + (2 + ii)] = b2;
          B[(i + jj) * S + (3 + ii)] = b3;
          B[(i + jj) * S + (4 + ii)] = b4;
          B[(i + jj) * S + (5 + ii)] = b5;
          B[(i + jj) * S + (6 + ii)] = b6;
          B[(i + jj) * S + (7 + ii)] = b7;
          b0 = A[((i + BS / 2) + ii) * S + (0 + jj)];
          b1 = A[((i + BS / 2) + ii) * S + (1 + jj)];
          b2 = A[((i + BS / 2) + ii) * S + (2 + jj)];
          b3 = A[((i + BS / 2) + ii) * S + (3 + jj)];
          b4 = A[((i + BS / 2) + ii) * S + (4 + jj)];
          b5 = A[((i + BS / 2) + ii) * S + (5 + jj)];
          b6 = A[((i + BS / 2) + ii) * S + (6 + jj)];
          b7 = A[((i + BS / 2) + ii) * S + (7 + jj)];
          t = b0, b0 = B[(i + jj) * S + (4 + ii)], B[(i + jj) * S + (4 + ii)] = t;
          t = b1, b1 = B[(i + jj) * S + (5 + ii)], B[(i + jj) * S + (5 + ii)] = t;
          t = b2, b2 = B[(i + jj) * S + (6 + ii)], B[(i + jj) * S + (6 + ii)] = t;
          t = b3, b3 = B[(i + jj) * S + (7 + ii)], B[(i + jj) * S + (7 + ii)] = t;
          B[((i + BS / 2) + jj) * S + (0 + ii)] = b0;
          B[((i + BS / 2) + jj) * S + (1 + ii)] = b1;
          B[((i + BS / 2) + jj) * S + (2 + ii)] = b2;
          B[((i + BS / 2) + jj) * S + (3 + ii)] = b3;
          B[((i + BS / 2) + jj) * S + (4 + ii)] = b4;
          B[((i + BS / 2) + jj) * S + (5 + ii)] = b5;
          B[((i + BS / 2) + jj) * S + (6 + ii)] = b6;
          B[((i + BS / 2) + jj) * S + (7 + ii)] = b7;
        }

        for(int i = 0; i < BS / 2; ++i)
          for(int j = i + 1; j < BS / 2; ++j)
          {
            int Bijjjii = B[((i + BS / 2) + jj) * S + (j + ii)];
            int Bjjjiii = B[((j + BS / 2) + jj) * S + (i + ii)];
            B[((j + BS / 2) + jj) * S + (i + ii)] = Bijjjii;
            B[((i + BS / 2) + jj) * S + (j + ii)] = Bjjjiii;
          }
        for(int i = 0; i < BS / 2; ++i)
          for(int j = i + 1; j < BS / 2; ++j)
          {
            int Bijjjii = B[((i + BS / 2) + jj) * S + ((j + BS / 2) + ii)];
            int Bjjjiii = B[((j + BS / 2) + jj) * S + ((i + BS / 2) + ii)];
            B[((j + BS / 2) + jj) * S + ((i + BS / 2) + ii)] = Bijjjii;
            B[((i + BS / 2) + jj) * S + ((j + BS / 2) + ii)] = Bjjjiii;
          }
        for(int i = 0; i < BS / 2; ++i)
          for(int j = i + 1; j < BS / 2; ++j)
          {
            int Bijjjii = B[(i + jj) * S + (j + ii)];
            int Bjjjiii = B[(j + jj) * S + (i + ii)];
            B[(j + jj) * S + (i + ii)] = Bijjjii;
            B[(i + jj) * S + (j + ii)] = Bjjjiii;
          }
        for(int i = 0; i < BS / 2; ++i)
          for(int j = i + 1; j < BS / 2; ++j)
          {
            int Bijjjii = B[(i + jj) * S + ((j + BS / 2) + ii)];
            int Bjjjiii = B[(j + jj) * S + ((i + BS / 2) + ii)];
            B[(j + jj) * S + ((i + BS / 2) + ii)] = Bijjjii;
            B[(i + jj) * S + ((j + BS / 2) + ii)] = Bjjjiii;
          }
      }
    }
}

void transpose_8_8(int *A, int *B, int S, int BS)
{
  BS = 4;
  const int MN = 8;
  for(int ii = 0; ii < MN; ii += BS)
    for(int jj = 0; jj < MN; jj += BS)
      for(int i = ii; i < ii + BS; ++i)
      {
        int b0, b1, b2, b3;
        b0 = A[i * S + (jj + 0)];
        b1 = A[i * S + (jj + 1)];
        b2 = A[i * S + (jj + 2)];
        b3 = A[i * S + (jj + 3)];
        B[(jj + 0) * S + i] = b0;
        B[(jj + 1) * S + i] = b1;
        B[(jj + 2) * S + i] = b2;
        B[(jj + 3) * S + i] = b3;
      }
}
