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
const int TERMINATE_REQUEST = 6;

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
	int rank, size, data = 0, request = -1, writersCount = 3, index = 0, rc = 0, readyToRecieve = 1, respond = 0, wtime = 0, iterations = 2;
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
	if (cmdOptionExists(argv, argv + argc, "-i"))
	{
		char * wcount = getCmdOption(argv, argv + argc, "-i");
		iterations = atoi(wcount);
	}

	//server
	if (!rank) {

		int curReadersCount = 0;
		int curWriterRank;
		bool isBusy = false;
		int activeProcess = size - 1;

		cout << "Starting Server" << endl;
		cout << "Writers count = " << writersCount << endl;
		cout << "R/W iterations = " << iterations << endl;

		while (true) {
			if (readyToRecieve) {
				MPI_Recv(&request, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST, MPI_COMM_WORLD, &status);
				readyToRecieve = 0;
			}
			else {	
				if (!index) {
					if (request == WRITE_REQUEST) {
						curWriterRank = status.MPI_SOURCE;
						if (!rc) {
							respond = 1;
							MPI_Send(&respond, 1, MPI_INT, curWriterRank, SERVER, MPI_COMM_WORLD);
							cout << "--------DATA OVERWRITE--------" << endl;
							cout << "Process " << curWriterRank << " is writing" << endl;
							MPI_Recv(&data, 1, MPI_INT, MPI_ANY_SOURCE, status.MPI_SOURCE, MPI_COMM_WORLD, &status);
							cout << "data = " << data << endl;
						}
						else {
							respond = 0;
							MPI_Send(&respond, 1, MPI_INT, curWriterRank, SERVER, MPI_COMM_WORLD);
							cout << "--------DATA OVERWRITE--------" << endl;
							cout << "Process " << curWriterRank << " : ACCESS DENIED" << endl;
						}
					}

					if (request == READ_REQUEST) {
						cout << "------------------------------" << endl;
						cout << "Process " << status.MPI_SOURCE << " reading..." << endl;
						rc++;
						MPI_Isend(&data, 1, MPI_INT, status.MPI_SOURCE, READ_REQUEST, MPI_COMM_WORLD, &request);
						cout << "Server sent data to " << status.MPI_SOURCE << ", data = " << data << endl;
						cout << "Current readers count : " << rc << endl;
					}

					if (request == FINISH_READ) {
						rc--;
						cout << "------------------------------" << endl;
						cout << "Process " << status.MPI_SOURCE << " finished reading " << endl;;
						cout << "Current readers count : " << rc << endl;
					}

					if (request == TERMINATE_REQUEST) {
						cout << "------------------------------" << endl;
						cout << "Process " << status.MPI_SOURCE << " wants to terminate" << endl;
						activeProcess--;
						cout << "Active process : " << activeProcess << endl;
					}
					if (!activeProcess) {
						cout << "SHUTTING DOWN..." << endl;
						break;
					}
					readyToRecieve = 1;
				}
			}
		}
	}

	//writer
	if (rank > 0 && rank <= writersCount) {
		int rankSleepTime = 0;

		request = WRITE_REQUEST;
		srand((unsigned)time(&t));
		data = rank;
		for (int i = 0; i < rank; i++) rankSleepTime = rand() % 10;
		while (iterations) {
			if (rand() % 100 < 5) {
				MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
				MPI_Recv(&respond, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
				if (respond) {
					MPI_Send(&data, 1, MPI_INT, SERVER, rank, MPI_COMM_WORLD);
				}
				Sleep(15000 + 1000 * rankSleepTime);
				iterations--;
			}
		}
		request = TERMINATE_REQUEST;
		MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
	}

	//reader
	if (rank > writersCount) {
		int rankSleepTime = 0;

		srand((unsigned)time(&t));
		for (int i = 0; i < rank; i++) rankSleepTime = rand() % 10;
		while (iterations) {
			if (rand() % 100 < 10) {
				request = READ_REQUEST;
				MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
				MPI_Recv(&data, 1, MPI_INT, SERVER, READ_REQUEST, MPI_COMM_WORLD, &status);
				Sleep(10000 + 500 * rankSleepTime);
				request = FINISH_READ;
				MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
				Sleep(10000 + 500 * rankSleepTime);
				iterations--;
			}
		}
		request = TERMINATE_REQUEST;
		MPI_Send(&request, 1, MPI_INT, SERVER, REQUEST, MPI_COMM_WORLD);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
}
