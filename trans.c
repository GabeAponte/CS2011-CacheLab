//Hunter Trautz and Gabriel Aponte
//Team Name: hctautz-gaaponte

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

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  //stores the size of the block that will be used for each specific case
  //32x32 blockSize = 8, 64x64 blockSize = 4, any other size matrix uses blockSize = 16
  int blockSize;
  //used to iterate over blocks in column major order
  int columnBlock, rowBlock;
  //used to iterate through each individual block, as opposed to over them
  int column, row;
  //variable used to store the row number whenever the diagonal line of the matrix is touched
  int diagonal;
  //tmporary variable used to store elements for later re-assignment within the matrix
  int tmp;

  //32x32 case
  if(N == 32){
    //Optimal block size, will only be cold misses
    blockSize = 8;

    for(columnBlock = 0; columnBlock < N; columnBlock +=blockSize){
      for(rowBlock = 0; rowBlock < N; rowBlock +=blockSize){
        for(row = rowBlock; row < rowBlock + blockSize; row++){
          for(column = columnBlock; column < columnBlock + blockSize; column++){
            /*row and column numbers are equal meaning we hit the diagonal line */
              if(row == column){
                //store the current value for later re-assignment
                tmp = A[row][column];
                diagonal = row;
              } else {
                  //if the row and column numbers are not equal we can transpose normally
                  B[column][row] = A[row][column];
              }
          }
          if(columnBlock == rowBlock){
            B[diagonal][diagonal] = tmp;
          }
        }
      }
    }
  } else if(N == 64){
    //variables used for assignmnets within the 64x64 case because we have to assign
    //elements in each row individually and are unable to modify the matrix A
    int a0, a1, a2, a3, a4;
    //Optimal blocksize
    blockSize = 4;
    for(row = 0; row < N; row+=blockSize){
      for(column = 0; column < M; column+=blockSize){

        //store elements from A because we are unable to modify them
        a0 = A[row][column];
        a1 = A[row+1][column];
        a2 = A[row+2][column];
        a3 = A[row+2][column+1];
        a4 = A[row+2][column+2];

        //B[column+3]
        B[column+3][row] = A[row][column+3];
        B[column+3][row+1] = A[row+1][column+3];
        B[column+3][row+2] = A[row+2][column+3];

        //B[column+2]
        B[column+2][row] = A[row][column+2];
        B[column+2][row+1] = A[row+1][column+2];
        B[column+2][row+2] = a4;
        a4 = A[row+1][column+1];

        //B[column+1]
        B[column+1][row] = A[row][column+1];
        B[column+1][row+1] = a4;
        B[column+1][row+2] = a3;

        //B[column]
        B[column][row] = a0;
        B[column][row+1] = a1;
        B[column][row+2] = a2;

        B[column][row+3] = A[row+3][column];
        B[column+1][row+3] = A[row+3][column+1];
        B[column+2][row+3] = A[row+3][column+2];
        a0 = A[row+3][column+3];

        B[column+3][row+3] = a0;
      }
    }
  }
  else /* random matrix */{
    //optimal blocksize
    blockSize = 16;

    for(columnBlock = 0; columnBlock < M; columnBlock += blockSize){
      for(rowBlock = 0; rowBlock < N; rowBlock += blockSize){
        //because we do not know the size of the matrix we cannot assume that all blocks will be square
        //and therefore must check that we do not run off the matrix through each iteration
        for(row = rowBlock; (row < rowBlock + blockSize) && (row < N); row++){
          for(column = columnBlock; (column < columnBlock + blockSize) && (column < M); column++){
            /*row and column numbers are equal meaning we hit the diagonal line */
            if(row == column){
              //store the current value for later re-assignment
              tmp = A[row][column];
              diagonal = row;
            } else {
              //if the row and column numbers are not equal we can transpose normally
              B[column][row] = A[row][column];
            }
          }
          if(rowBlock == columnBlock){
            B[diagonal][diagonal] = tmp;
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
