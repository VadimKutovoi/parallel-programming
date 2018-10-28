#include "mpi.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void generate_matr(double* matr,int row_c, int column_c) {
	for (int i = 0; i < row_c * column_c; i++) {
		matr[i] = rand() % 10;
	}
}

void print_matr(double* matr, int row_c, int column_c) {
	for (int i = 1; i < row_c * column_c + 1; i++) {
		std::cout << matr[i - 1] << " ";
		if (i%column_c == 0) {
			std::cout << std::endl;
		}
	}
}


void main(int argc, char *argv[])
{
	//parallel program
	MPI_Init(&argc, &argv);

	int rank, size, rows = 5, columns = 5, srows = 1, sumc = 0;
	double *matrix = NULL, *recv_row = NULL, *sum = NULL, *lsum = NULL;
	double times;

	int *sendcounts, *displs, *recvcounts;

	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	int rem = 0;

	if (rank == 0) {
		if (argc == 2) {
			columns = rows = atoi(argv[1]);
		}
		else {
			rows = columns = size;
		}

		std::cout << "Generating matrix.." << std::endl;

		matrix = new double[columns * rows];
		sum = new double[rows];

		generate_matr(matrix, rows, columns);
		//print_matr(matrix, rows, columns);

		srows = (rows / size) == 0 ? 1 : rows/size;
		std::cout << "srows = " << srows << std::endl;

		times = MPI_Wtime();
	}

	MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&columns, 1, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(&srows, 1, MPI_INT, 0, MPI_COMM_WORLD);

	rem = (rows < size) ? 0 : rows % size;
	sendcounts = new int[size];
	recvcounts = new int[size];
	displs = new int[size];

	sendcounts[0] = columns * (srows + rem);
	recvcounts[0] = srows + rem;
	displs[0] = 0;
	for (int i = 1; i < size; i++) {
		sendcounts[i] = columns * srows;
		displs[i] = displs[i - 1] + sendcounts[i - 1];
		recvcounts[i] = srows;
	}

	recv_row = new double[sendcounts[rank]];

	MPI_Scatterv(matrix, sendcounts, displs, MPI_DOUBLE, recv_row, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

	//for (int i = 0; i < sendcounts[rank]; i++) {
	//	std::cout << recv_row[i] << " ";
	//}
	//std::cout << std::endl;

	lsum = new double[sendcounts[rank]/columns];
	for (int i = 0; i < sendcounts[rank] / columns; i++){
		lsum[i] = 0;
		for (int j = i * columns; j < sendcounts[rank]; j++) {
			lsum[i] += recv_row[j];
			if ((j + 1) % columns == 0) break;
		}
	}

	//for (int i = 0; i < sendcounts[rank] / columns; i++)
	//	std::cout << "rank " << rank << " lsum = " << lsum[i] << " ";
	//std::cout << std::endl;

	int *displs2 = new int[size] {0};
	
	for (int i = 1; i < size; i++) {
		displs2[i] = displs2[i - 1] + recvcounts[i - 1];
	}

	//if (!rank) {
	//	for (int i = 0; i < rows; i++) {
	//		std::cout << "disp[" << i << "] = " << displs2[i] << std::endl;
	//	}
	//}

	MPI_Gatherv(lsum, sendcounts[rank]/columns, MPI_DOUBLE, sum, recvcounts, displs2, MPI_DOUBLE, 0, MPI_COMM_WORLD);
	
	if (!rank) {
		std::cout << "Parallel time = " << MPI_Wtime() - times << std::endl;
		for (int i = 0; i < rows; i++) {
			std::cout << "sum[" << i << "] = " <<sum[i] << std::endl;
		}
	}

	//sequential program

	if (rank == 0) {
		for (int j = 0; j < columns; j++) sum[j] = 0;

		std::cout << std::endl;

		times = MPI_Wtime();

		int i = 0;
		for (int j = 0; j < columns * rows; j++) {
			sum[i] += matrix[j];
			if ((j + 1) % columns == 0) i++;
		}
	
		std::cout << "Sequental time = " << MPI_Wtime() - times << std::endl;

		for (int i = 0; i < rows; i++) {
			std::cout << "sum2[" << i << "] = " << sum[i] << std::endl;
		}

		delete[] matrix, sum;
	}

	MPI_Barrier(MPI_COMM_WORLD);
	delete[] lsum, recv_row, displs2, displs, sendcounts, recvcounts;

	MPI_Finalize();
}