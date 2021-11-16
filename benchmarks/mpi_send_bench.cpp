#include <ratio>
#include <mpi.h>
#include <chrono>
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
    int c_rank;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);


    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <iterations>" << endl;
        return -1;
    }
    const long int iterations = atoi(argv[1]);


    auto t1 = chrono::high_resolution_clock::now();
    double num = 2.0;
    for (auto i = 0; i < iterations; i++) {
        if (c_rank == 0) {
            num += 1.0;
            MPI_Send(&num, 1, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
        } else if(c_rank == 1) {
            MPI_Recv(&num, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, nullptr);
        }
    }

    auto t2 = chrono::high_resolution_clock::now();

    chrono::duration<double, nano> dur = t2 - t1;
    double avg = dur.count() / (double) iterations;
    cout << "MPI_Send&Recv took on average " << avg << "ns" << endl;



    MPI_Finalize();
}
