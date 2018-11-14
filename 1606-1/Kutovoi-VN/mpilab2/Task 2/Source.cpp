#define _CRT_SECURE_NO_WARNINGS
#include "mpi.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <Windows.h>
#include <algorithm>

const int SERVER = 0;
const int READ_REQUEST = 1;
const int FINISH_READ = 3;
const int WRITE_REQUEST = 4;
const int REQUEST = 5;

using namespace std;

char* getCmdOption(char ** begin, char ** end, const std::string & option)
{
	char ** itr = std::find(begin, end, option);
	if (itr != end && ++itr != end)
	{
		return *itr;
	}
	return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
	return std::find(begin, end, option) != end;
}

void main(int argc, char *argv[])
{
	int rank, size, data = 0, request = -1, writersCount = 3, index = 0, rc = 0, readyToRecieve = 1, respond = 0;
	time_t t;
	MPI_Status status;
	MPI_Request mpiRequest;

	MPI_Init(NULL, NULL);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (cmdOptionExists(argv, argv + argc, "-w"))
	{
		char * wcount = getCmdOption(argv, argv + argc, "-w");
		writersCount = atoi(wcount);
	}

	//server
	if (!rank) {

		int curReadersCount = 0;
		int curWriterRank;
		bool isBusy = false;

		cout << "Starting Server" << endl;
		cout << "Writers count = " << writersCount << endl;
		
		while (true) {
			if (readyToRecieve) {
				MPI_Irecv(&request, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &mpiRequest);
				readyToRecieve = 0;
			}
			else {
				MPI_Test(&mpiRequest, &index, &status);
				//cout << "SERVER RECIEVED REQUEST " << request << endl;

				if ((index != 0) && (request == WRITE_REQUEST)){
					curWriterRank = status.MPI_SOURCE;
					if (!rc) {
						respond = 1;
						MPI_Send(&respond, 1, MPI_INT, curWriterRank, SERVER, MPI_COMM_WORLD);
						cout << "--------DATA OVERWRITE--------" << endl;
						cout << "Process " << curWriterRank << " is writing" << endl;
						MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, status.MPI_SOURCE, MPI_COMM_WORLD, &status);
						cout << "data = " << data << endl;
						readyToRecieve = 1;
					}
					else {
						respond = 0;
						MPI_Send(&respond, 1, MPI_INT, curWriterRank, SERVER, MPI_COMM_WORLD);
						cout << "--------DATA OVERWRITE--------" << endl;
						cout << "Process " << curWriterRank << " : ACCESS DENIED" << endl;
						readyToRecieve = 1;
					}
				}

				if ((index != 0) && (request == READ_REQUEST)) {
					cout << "------------------------------" << endl;
					cout << "Process " << status.MPI_SOURCE << " reading..." << endl;
					rc++;
					MPI_Isend(&data, 1, MPI_INT, status.MPI_SOURCE, READ_REQUEST, MPI_COMM_WORLD, &request);
					cout << "Server sent data to " << status.MPI_SOURCE << ", data = " << data << endl;
					readyToRecieve = 1;
					cout << "Current readers " << rc << endl;
				}

				if ((index != 0) && (request == FINISH_READ)) {
					rc--;
					cout << "------------------------------" << endl;
					cout << "Process " << status.MPI_SOURCE << " finished reading " << endl;
					readyToRecieve = 1;
					cout << "Current readers " << rc << endl;
				}
			}
		}
	}

	//writer
	if(rank > 0 && rank <= writersCount){
		request = WRITE_REQUEST;
		srand((unsigned)time(&t));
		data = rank;
		while (true) {
			if (rand() % 100 < 5) {
				MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
				MPI_Recv(&respond, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
				if (respond) {
					MPI_Send(&data, 1, MPI_INT, SERVER, rank, MPI_COMM_WORLD);
				}
				Sleep(15000 + 1000 * rank);
			}
		}
	}

	//reader
	if (rank > writersCount) {
		while (true) {
			if (rand() % 100 < 10) {
				request = READ_REQUEST;
				MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
				MPI_Recv(&data, 1, MPI_INT, SERVER, READ_REQUEST, MPI_COMM_WORLD, &status);
				Sleep(10000 + 500 * rank);
				request = FINISH_READ;
				MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
				Sleep(10000 + 500 * rank);
			}
		}
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
}