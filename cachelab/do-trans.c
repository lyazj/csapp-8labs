#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void transpose_submit(int M, int N, int A[N][M], int B[M][N]);
int is_transpose(int M, int N, int A[N][M], int B[M][N]);

#define N 32
#define M 32

int A[N][M], B[M][N];

int main(void)
{
  srand(time(NULL));
  for(int i = 0; i < N; ++i)
    for(int j = 0; j < M; ++j)
      A[i][j] = rand();
  transpose_submit(M, N, A, B);
}
