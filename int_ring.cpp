// mpirun -np 4 ./scan
#include <algorithm>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <mpi.h>
#include <cassert>  

int main(int argc, char* argv[]) {
    long N = 10000;
    int rank, size;
    long* A = (long*) malloc(N * sizeof(long)); 
    long* B0 = (long*) malloc(N * sizeof(long));
    double s1,s2,e1,e2;

    MPI_Init(&argc, &argv);
    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);

    long chunksize = ceil(N / double(size)); 
    long* B1 = (long*) malloc(chunksize * sizeof(long));
    long* B2 = (long*) malloc(N * sizeof(long));
    long* receive = (long*) malloc(chunksize * sizeof(long));
    long* correction = (long*) malloc(size * sizeof(long));
    for (long i = 0; i < chunksize; i++) B1[i] = 0;
    for (long i = 0; i < N; i++) B2[i] = 0;
    for (long i = 0; i < chunksize; i++) receive[i] = 0;
    for (long i = 0; i < size; i ++) correction[i] = 0;

        if (rank == 0){ 
        for (long i = 0; i < N; i++) A[i] = rand(); ;
        for (long i = 0; i < N; i++) B0[i] = 0;
        s1 = MPI_Wtime();
        B0[0] = A[0];
        for (long i = 1; i < N; i++) {
        B0[i] = B0[i-1] + A[i]; }
        e1 = MPI_Wtime();
        printf("sequential-scan = %fs\n", e1-s1);
    }


    s2 = MPI_Wtime();
    MPI_Scatter(A, chunksize, MPI_LONG, receive, chunksize, MPI_LONG, 0, comm);
    

    B1[0] = receive[0]; 
    for(int j = 1; j < chunksize; j++){
        B1[j] = B1[j - 1] + receive[j];
    }
    correction[rank] = B1[chunksize-1];

    for (int i = 0; i < size; i++){
      MPI_Bcast(&correction[i], 1, MPI_LONG, i, MPI_COMM_WORLD);
    }

    long prev = 0; 
    for (int i = 0; i < rank; i++) prev += correction[i];
    for (int i = 0; i < chunksize; i++) B1[i] += prev;
    free(correction);

    MPI_Gather(B1, chunksize, MPI_LONG, B2, chunksize, MPI_LONG, 0, MPI_COMM_WORLD);
    
    e2 = MPI_Wtime();


        if (rank == 0) {
        printf("parallel-scan   = %fs\n", e2-s2);
            long err = 0;
            for (long i = 0; i < N; i++) err = std::max(err, std::abs(B0[i] - B2[i]));
            printf("error = %ld\n", err);
        }

    
    free(A);
    free(B0);
    free(B1);
    free(B2);
    free(receive);
    MPI_Finalize();
    return 0;
}
