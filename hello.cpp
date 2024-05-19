/*
MPI Program, hello and response
*/

#include <stdio.h>
#include <mpi.h>

#define MASTER 0

int main(int argc, char* argv[]){
  int my_rank;
  int num_processes;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

  MPI_Comm_size(MPI_COMM_WORLD, &num_processes);

  printf("Process %d: There is a total of %d \n", my_rank, num_processes);

  if(my_rank == MASTER)
  {
    int dest = 1;
    int tag = 0;
    int count = 1;
    for(int i = 1; i < num_processes; i++)
    {
      MPI_Send(&i, count, MPI_INT, i, tag, MPI_COMM_WORLD);
      printf("Process %d: Sent %d to process %d \n", my_rank, i, dest);
    }

  }
  else
  {
    int tag = 0;
    int count = 1;
    int buffer;
    MPI_Recv(&buffer, count, MPI_INT, MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Process %d: Received %d from process %d \n", my_rank, buffer, MASTER);
  }

  MPI_Finalize();
  return 0;
}
