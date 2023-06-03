#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    srand(time(NULL));
  if (argc != 2) {
    printf("Usage: %s <integer>\n", argv[0]);
    return 1;
  }

  // Convert input argument to integer
  int input = atoi(argv[1]);

  // Calculate size of arrays
  int size = input;

  // Allocate memory for arrays
  int **A = (int **)malloc(size * sizeof(int *));
  int **B = (int **)malloc(size * sizeof(int *));
  int **C = (int **)malloc(size * sizeof(int *));
  for (int i = 0; i < size; i++) {
    A[i] = (int *)malloc(size * sizeof(int));
    B[i] = (int *)malloc(size * sizeof(int));
    C[i] = (int *)malloc(size * sizeof(int));
  }

  // Initialize arrays
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      A[i][j] = rand() % 100 + 1;
      B[i][j] = rand() % 100 + 1;
    }
  }
  // for (int i = 0; i < size; i++) {
  //   for (int j = 0; j < size; j++) {
  //     A[i][j] = rand() % 100;
  //     B[i][j] = rand() % 100;
  //   }
  // }
  // C = A*B;
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++){
      C[i][j] = 0;
      for (int k = 0; k < size; k++){
        C[i][j] += A[i][k] * B[k][j];
      }
    }
  }

  // Write arrays to files
  FILE *fa = fopen("A.txt", "w");
  FILE *fb = fopen("B.txt", "w");
  FILE *fc = fopen("C.txt", "w");
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      fprintf(fa, "%d", A[i][j]);
      fprintf(fb, "%d", B[i][j]);
      fprintf(fc, "%d", C[i][j]);
      if (j < size - 1) {
        fprintf(fa, ",");
        fprintf(fb, ",");
        fprintf(fc, ",");
      }
    }
    fprintf(fa, "\n");
    fprintf(fb, "\n");
    fprintf(fc, "\n");
  }
  fclose(fa);
  fclose(fb);
  fclose(fc);
  // Free memory
  for (int i = 0; i < size; i++) {
    free(A[i]);
    free(B[i]);
    free(C[i]);
  }
  free(A);
  free(B);
  free(C);

  return 0;
}
