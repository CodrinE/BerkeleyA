/*
 ============================================================================
 Name        : Berkeley.c
 Author      : Codrin
 Version     :
 Copyright   : Your copyright notice
 Description : Calculate Pi in MPI
 ============================================================================
 */
//build project command and linking required libraries to create the executable: mpicc -o Berkeley Berkeley.c -lm
//run project command: mpirun -np 4 Berkeley Berkeley.txt

#include <bits/types/FILE.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
	int rank, i, size, nrOfProcesses;
	int iFlag[100];
	double time1, time2;
	int machines[6]={1, 106430, 106455, 106400, 1064500, 66};  //time in milliseconds; array contains [master process, time, time, time, time, threshold]
	long double masterTime, clockProcMs, adjTimeMs, average, t, pMs[100], sumIFlags, diffTs;
	MPI_Status status;

	MPI_Init(&argc, &argv); // Initializing MPI
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); // rank of each process
	MPI_Comm_size(MPI_COMM_WORLD, &size); // total number of processes

	clockProcMs = machines[rank + 1]; //current process time

	nrOfProcesses = sizeof(machines)/sizeof(int) - 2;

	if(size != nrOfProcesses)
	{
		printf("Error: Number of processes must be %d\n", nrOfProcesses);
		MPI_Finalize();
		return 0;
	}
		if(rank == machines[0]) // master process
		{
			printf("I am process with rank %d acting as the master process\n", rank);
			masterTime = machines[machines[0] + 1];
			printf("Master process is sending time %Lf ms\n", masterTime);
			time1=MPI_Wtime();
			MPI_Bcast(&masterTime, 1, MPI_LONG_DOUBLE, machines[0], MPI_COMM_WORLD); // broadcasting master time
			MPI_Barrier(MPI_COMM_WORLD); // barrier is used so that all processors wait for each other after broadcasting is done

			for(i=0; i<size-1; i++)
			{
				MPI_Recv(&t, 1, MPI_LONG_DOUBLE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
				time2 = MPI_Wtime();
				diffTs =  t - masterTime + ((time1 + time2) * 500); //time difference between master and slave

				pMs[status.MPI_SOURCE] = diffTs;
				printf("Process %d has received time differential value of %Lf ms\n", rank, diffTs);
				if(abs(diffTs) > masterTime + machines[nrOfProcesses + 1]) // check if  the time difference has to be ignored
				{
					iFlag[status.MPI_SOURCE] = 0; // iFlag is used to tell if the time is ignored, 0 if ignored, 1 if not ignored
					printf("Master process is ignoring clock from process %d as it is greater than the imposed limit\n", status.MPI_SOURCE);
				}
				else
					iFlag[status.MPI_SOURCE] = 1;
			}

			average = 0;
			sumIFlags = size;

			for(i=0; i<size; i++)
				if(i != rank)
				{
					average = average + ((pMs[i])*iFlag[i]); // multiply with iFlag to ignore the processes
					sumIFlags = sumIFlags + iFlag[i] - 1; // remove the ignored processes from total processes to calculate average
				}
			average = average / sumIFlags; // average value
			printf("Time differential average is %Lf ms\n", average);

			clockProcMs = clockProcMs + average; // adjust master time value
			printf("Process %d time has changed to %Lf ms\n",rank, clockProcMs);
			for(i=0; i<size; i++)
				if(i != rank)
				{
					adjTimeMs = average - pMs[i]; // time to be adjusted for each process
					printf("Master process is sending the clock adjustment value of %Lf ms to process %d\n", adjTimeMs, i);
					MPI_Send(&adjTimeMs, 1, MPI_LONG_DOUBLE, i, 1, MPI_COMM_WORLD); // send time to be adjusted to respective processes
				}
		}
		else
		{
			MPI_Bcast(&masterTime, 1, MPI_LONG_DOUBLE, machines[0], MPI_COMM_WORLD); // receive broadcasted master time
			MPI_Barrier(MPI_COMM_WORLD);
			printf("Process %d has received time %Lf ms\n", rank, masterTime);

			printf("Process %d is sending time value of %Lf ms to process %d\n", rank, clockProcMs, machines[0]);
			MPI_Send(&clockProcMs, 1, MPI_LONG_DOUBLE, machines[0], 0, MPI_COMM_WORLD); // send time difference to master
			MPI_Recv(&adjTimeMs, 1, MPI_LONG_DOUBLE, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status); // receive time to be adjusted from master
			printf("Process %d has received the clock adjustment value of %Lf\n", rank, adjTimeMs);
			clockProcMs = clockProcMs + adjTimeMs; // adjust the time
			printf("Process %d time has changed to %Lf ms\n",rank, clockProcMs);
		}


	MPI_Finalize();
	return 0;
}
