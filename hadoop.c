#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

void readFile(char *filename, char **buffer)
{
  FILE *fp;
  long lSize;

  fp = fopen(filename, "rb");
  if (!fp)
    perror(filename), exit(1);

  fseek(fp, 0L, SEEK_END);
  lSize = ftell(fp);
  rewind(fp);

  /* allocate memory for entire content */
  *buffer = calloc(1, lSize + 1);
  if (!buffer)
    fclose(fp), fputs("memory alloc fails", stderr), exit(1);

  /* copy the file into the buffer */
  if (1 != fread(*buffer, lSize, 1, fp))
    fclose(fp), free(*buffer), fputs("entire read fails", stderr), exit(1);

  /* do your work here, buffer is a string contains the whole text */

  fclose(fp);
}
void countLines(char *buffer, int *lines)
{
  int i = 0;
  int count = 0;
  while (buffer[i] != '\0')
  {
    if (buffer[i] == '\n')
    {
      count++;
    }
    i++;
  }
  *lines = count;
}
void fillMatrix(char *buffer, int *matrix, int arraySize)
{
  int i = 0;
  int j = 0;
  int k = 0;
  int number = 0;
  while (buffer[i] != '\0')
  {
    if (buffer[i] == '\n')
    {
      matrix[j * arraySize + k] = number;
      number = 0;

      j++;
      k = 0;
    }
    else if (buffer[i] == ',')
    {
      matrix[j * arraySize + k] = number;
      number = 0;
      k++;
    }
    else
    {
      number = number * 10 + (buffer[i] - '0');
    }
    i++;
  }
}
void printMatrix(int *matrix, int arraySize)
{
  int total = 0;
  for (int i = 0; i < arraySize; i++)
  {
    for (int j = 0; j < arraySize; j++)
    {
      // printf("%d, ", matrix[i * arraySize + j]);
      total += 1;
    }
    // printf("\n");
  }
  // printf("Total: %d\n", total);
}

int *map(int *matrixA, int *matrixB, int arraySize, int initialRow, int nRows)
{
  if (nRows == 0)
  {
    return NULL;
  }

  int allKeyValuePairsSize = nRows*arraySize*arraySize;
  int* allKeyValuePairs = malloc(allKeyValuePairsSize * sizeof(int));

  int counter = 0;
  for (int i = 0; i < nRows; i++)
  {
    for (int j = 0; j < arraySize; j++)
    {
      for (int k = 0; k < arraySize; k++)
      {
        // int result = matrixA[i * arraySize + k] * matrixB[k * arraySize + j];
        // store result of multiplication where B is transposed
        int result = matrixA[i * arraySize + k] * matrixB[j * arraySize + k]; 
        allKeyValuePairs[counter] = result;
        counter++;
      }
      // printf("\n");
    }
  }

  // // printf("All key value pairs: ");
  // for (int i = 0; i < allKeyValuePairsSize; i++)
  // {
  //   // printf("%d ", allKeyValuePairs[i]);
  // }
  // // printf("\n");
  // // printf("Number of key value pairs: %d\n", counter);
  return allKeyValuePairs;
}

int* reduce(int* reducerInput, int pairsPerReducer, int arraySize) {  
    int* reducerOutput = calloc(pairsPerReducer, sizeof(int));

    for (int i=0; i<pairsPerReducer; i++){
      for (int j=0; j<arraySize; j++){
        reducerOutput[i] += reducerInput[i*arraySize + j];
      }
    }
  
    // for (int i=0; i<pairsPerReducer; i++){
    //   // printf("%d\n", reducerOutput[i]);
    // }

    return reducerOutput;
}

// Determine how much to send to each reducer
void shuffle(int* mappedString, int totalPairs, int totalReducers, int* sendCounts, int* displacements, int arraySize) {

  int pairsPerReducer = totalPairs / totalReducers;
  int extraPairs = totalPairs % totalReducers;

  sendCounts[0] = 0;
  displacements[0] = 0;

  for (int i=1; i<totalReducers+1; i++){
    if (i == 1) {
      displacements[i] = 0;
      sendCounts[i] = pairsPerReducer * arraySize;
    }
    else {
      displacements[i] = displacements[i-1] + sendCounts[i-1];
      sendCounts[i] = pairsPerReducer * arraySize;
    }
  }
  sendCounts[totalReducers] += extraPairs * arraySize;
}

int main(int argc, char *argv[])
{

  int rank, size, arraySize, nRows = 0, initialRow = 0;
  int *sendCounts = NULL;
  int *displacements = NULL;

  MPI_Init(&argc, &argv);               // Initialize MPI
  MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get process rank
  MPI_Comm_size(MPI_COMM_WORLD, &size); // Get number of processes

  int reducer_cutoff = ((size-1)/2)+1;

  char processorName[MPI_MAX_PROCESSOR_NAME];
  int len;
  MPI_Get_processor_name(processorName, &len);

  //-------------------------------------------------------------------------------------------------
  // ------------------------ Master Process ------------------------
  //-------------------------------------------------------------------------------------------------

  if (rank == 0) // The first process is the master
  {
    // printf("Hello from master process\n");
    printf("Number of mappers: %d\n", size - 1);
    printf("Number of reducers: %d\n", reducer_cutoff - 1);

    char *A = NULL;
    char *B = NULL;

    // Reading the Input Files
    readFile("A.txt", &A);
    readFile("B.txt", &B);

    // Counting the number of lines in the input files = number of rows/Cols in the matrix
    countLines(A, &arraySize);
    // printf("Array Dim: %d*%d\n", arraySize, arraySize);

    int *matrixA = malloc(arraySize * arraySize * sizeof(int));
    int *matrixB = malloc(arraySize * arraySize * sizeof(int));

    // Filling the matrix with the input values
    fillMatrix(A, matrixA, arraySize);
    fillMatrix(B, matrixB, arraySize);

    // printMatrix(matrixA, arraySize);

    // Take transpose of matrix B
    for (int i = 0; i < arraySize; i++)
    {
      for (int j = i + 1; j < arraySize; j++)
      {
        int temp = matrixB[i * arraySize + j];
        matrixB[i * arraySize + j] = matrixB[j * arraySize + i];
        matrixB[j * arraySize + i] = temp;
      }
    }

    // Fill the send counts and displacements
    int rowsPerProcess = arraySize / (size - 1);
    int extraRows = arraySize % (size - 1);

    sendCounts = malloc(size * sizeof(int));
    displacements = malloc(size * sizeof(int));

    for (int i = 0; i < size; i++)
    {
      if (i == 0)
      {
        sendCounts[i] = 0;
        displacements[i] = 0;
      }
      else
      {
        sendCounts[i] = rowsPerProcess * arraySize;
        displacements[i] = (i - 1) * sendCounts[i];
      }
    }
    sendCounts[size - 1] += extraRows * arraySize;

    // //Print the send counts and displacements
    // for (int i = 0; i < size; i++) {
    //   // printf("Send Count %d: %d\n", i, sendCounts[i]);
    //   // printf("Displacement %d: %d\n", i, displacements[i]);
    // }

    // send all the processes ,Array Sizes, tell them what their initial row is(It will help them in the mapping process) and how many rows they will get
    MPI_Bcast(&arraySize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(sendCounts, 1, MPI_INT, &nRows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(displacements, 1, MPI_INT, &initialRow, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Scatter the Arrays
    MPI_Scatterv(matrixA, sendCounts, displacements, MPI_INT, matrixA, rowsPerProcess * arraySize, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(matrixB, arraySize * arraySize, MPI_INT, 0, MPI_COMM_WORLD);

    for (int i=1;i<size;i++){
      printf("Task Map Assigned to Process %d \n",i);
    }
    
    int allKeyValuePairsSize = rowsPerProcess * arraySize * arraySize * (size - 1);
    
    int *allKeyValuePairs = malloc(allKeyValuePairsSize * sizeof(int));

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 1; i < size-1; i++)
    {
      MPI_Recv(allKeyValuePairs + rowsPerProcess * arraySize * arraySize * (i - 1), rowsPerProcess * arraySize * arraySize, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      printf("Process %d has completed task map\n",i );

    }
    MPI_Recv(allKeyValuePairs + rowsPerProcess * arraySize * arraySize * (size - 2), (rowsPerProcess + extraRows) * arraySize * arraySize, MPI_INT, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Process %d has completed task map\n",size-1 );

    printf("------------------------\n");


    int totalPairs = arraySize * arraySize;

    for (int i=reducer_cutoff; i<size; i++){
      sendCounts[i] = 0;
      displacements[i] = 0;
    }

    // Shuffling the key value pairs
    shuffle(allKeyValuePairs, totalPairs, reducer_cutoff-1, sendCounts, displacements, arraySize);

    // Send the sendcount to each reducer
    int recvCount;
    MPI_Scatter(sendCounts, 1, MPI_INT, &recvCount, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // Send the data to each reducer
    MPI_Scatterv(allKeyValuePairs, sendCounts, displacements, MPI_INT, allKeyValuePairs, 0, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i=1;i<reducer_cutoff;i++){
      printf("Task Reduce Assigned to Process %d \n",i);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // Now we have to control access of every reducer to the file
    for (int i=1; i<reducer_cutoff; i++) {
      // Send empty string to the process[i]
      MPI_Send("", 0, MPI_CHAR, i, 0, MPI_COMM_WORLD);
      // Now receive confirmation that the process[i] has finished
      MPI_Recv("", 0, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    free(allKeyValuePairs);

    free(sendCounts);
    free(displacements);

    free(A);
    free(B);

    free(matrixA);
    free(matrixB);


  }

  //-------------------------------------------------------------------------------------------------
  // ------------------------ Slave Process ------------------------
  //-------------------------------------------------------------------------------------------------

  else // Other processes are workers
  {

    MPI_Bcast(&arraySize, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(sendCounts, 1, MPI_INT, &nRows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(displacements, 1, MPI_INT, &initialRow, 1, MPI_INT, 0, MPI_COMM_WORLD);

    nRows = nRows / arraySize;
    initialRow = initialRow / arraySize;

    int *matrixA = malloc(nRows * arraySize * sizeof(int *));
    int *matrixB = malloc(arraySize * arraySize * sizeof(int *));

    MPI_Scatterv(matrixA, sendCounts, displacements, MPI_INT, matrixA, nRows * arraySize, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(matrixB, arraySize * arraySize, MPI_INT, 0, MPI_COMM_WORLD);

    //-----------------------------------------------------------------------------------

    int *allKeyValuePairs = map(matrixA, matrixB, arraySize, initialRow, nRows);
    
    printf("Process %d received task map on %s\n",rank,processorName );

    MPI_Barrier(MPI_COMM_WORLD);


    if (allKeyValuePairs != NULL)
    {
      MPI_Send(allKeyValuePairs, nRows * arraySize * arraySize, MPI_INT, 0, 0, MPI_COMM_WORLD);
      free(allKeyValuePairs);
    }
    else
    {
      // printf("Process %d: Error in mapping\n", rank);
      exit(-1);
    }
    
    free(matrixA);
    free(matrixB);

    // Get recvCount from the master
    int recvCount;
    MPI_Scatter(sendCounts, 1, MPI_INT, &recvCount, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank < reducer_cutoff)
    {
      int* reducerInput = malloc(recvCount * sizeof(int));
      MPI_Scatterv(NULL, sendCounts, displacements, MPI_INT, reducerInput, recvCount, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Barrier(MPI_COMM_WORLD);
      printf("Process %d received task reduce on %s\n",rank,processorName );
      // Determine pair count for this process
      int totalPairs = arraySize * arraySize;
      int totalReducers = reducer_cutoff - 1;
      int pairsPerReducer = totalPairs / totalReducers;
      int extraPairs = totalPairs % totalReducers;

      if (rank == totalReducers) {
        pairsPerReducer += extraPairs;
      }

      int* reducerOutput = reduce(reducerInput, pairsPerReducer, arraySize);

      MPI_Recv("", 0, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      FILE *file = fopen("output.txt", "a"); // Open file in append mode
      if (file == NULL) {
          printf("Error: could not open file.\n");
          return 1;
      }

      for (int i = 0; i < pairsPerReducer; i++) {
          fprintf(file, "%d", reducerOutput[i]);

          if (i % arraySize == arraySize - 1) {
              fprintf(file, "\n");
          } else {
              fprintf(file, ",");
          }
      }

      fclose(file); // Close the file


      MPI_Send("", 0, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

      free(reducerInput);
      free(reducerOutput);
    }
    else
    {
      MPI_Scatterv(NULL, sendCounts, displacements, MPI_INT, NULL, recvCount, MPI_INT, 0, MPI_COMM_WORLD);
      MPI_Barrier(MPI_COMM_WORLD);
      // MPI_Barrier(MPI_COMM_WORLD);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  if (rank == 0) {
    printf("\nJob has been Completed!\n");
  }
  MPI_Finalize(); // Finalize MPI
  return 0;
}