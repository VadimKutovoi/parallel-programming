#include "mpi.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void main(int argc, char *argv[])
{
	//parallel program
	MPI_Init(&argc, &argv);

	int rank, size, rows = 5, columns = 5;
	int **matrix = NULL, *recv_row = NULL, *sum = NULL;
	double times;
	MPI_Status status;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {
		std::cout << "Comm size = " << size << std::endl;
		
		std::cout << "Rows = ";
		std::cin >> rows; 
		
		std::cout << "Columns = ";
		std::cin >> columns;

		std::cout << "Generating matrix.." << std::endl;

		matrix = new int*[rows];
		sum = new int[rows];

		for (int i = 0; i < rows; i++) {
			matrix[i] = new int[columns];
			sum[i] = 0;
			for (int j = 0; j < columns; j++)
				matrix[i][j] = i + j;
		}

		for (int i = 0; i < rows; i++)
			for (int j = 0; j < columns; j++)
			{
				std::cout << matrix[i][j] << " ";
				if (j == columns - 1) std::cout << std::endl;
			}

	}

	times = MPI_Wtime();
	
	MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&columns, 1, MPI_INT, 0, MPI_COMM_WORLD);

	if (rank == 0) {
		std::cout << "Starting MPI_Send..." << std::endl;

		for (int i = 0; i < rows; i++) {
			if (i%size != 0)
				MPI_Send(matrix[i], columns, MPI_INT, i%size, i, MPI_COMM_WORLD);
		}

		std::cout << "Finished\n" << std::endl;
	}
	
	if(rank != 0) {
		recv_row = new int[columns];
		for (int i = rank; i < rows; i += size) {
			int row_sum = 0;

			MPI_Recv(recv_row, columns, MPI_INT, 0, i, MPI_COMM_WORLD, &status);
			std::cout << "Process " << rank << " recieved row" << std::endl;

			for (int j = 0; j < columns; j++)
				row_sum += recv_row[j];

			MPI_Send(&row_sum, 1, MPI_INT, 0, i, MPI_COMM_WORLD);
			std::cout << "Process " << rank << " sended row" << std::endl;
		}
	}
	else {
		for (int i = rank; i < rows; i += size)
		{
			for (int j = 0; j < columns; j++)
			{
				sum[i] += matrix[i][j];
			}
				
		}
	}

	if (rank == 0) {
		for (int i = 1; i < rows; i++) {
			if (i%size != 0) {
				MPI_Recv(&sum[i], 1, MPI_INT, MPI_ANY_SOURCE, i, MPI_COMM_WORLD, &status);				
			}
		}

		std::cout << "Time = " << MPI_Wtime() - times << std::endl;

		for (int i = 0; i < rows; i++) {
			std::cout << "sum[" << i << "] = " << sum[i] << std::endl;
		}
	}

	MPI_Finalize();

	for (int j = 0; j < columns; j++) sum[j] = 0;

	std::cout << std::endl;
	//sequential program

	time_t start, end;
	
	time(&start);

	for (int i = 0; i < rows; i++)
		for (int j = 0; j < columns; j++)
		{
			sum[i] += matrix[i][j];
		}

	time(&end);
	double seconds = difftime(end, start);
	std::cout << "sequential program time = " << seconds << std::endl;
	
	for (int i = 0; i < rows; i++) {
		std::cout << "sum2[" << i << "] = " << sum[i] << std::endl;
	}

	for (int i = 0; i < rows; i++) {
		delete[] matrix[i];
	}
	delete[] matrix, sum, recv_row;
}