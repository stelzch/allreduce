#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <cmath>
#include <cassert>
#include <unistd.h>

using namespace std;

int c_rank, c_size;


void attach_debugger(int rank) {
    if (c_rank != rank) return;
    bool attached = false;

    cout << "Waiting for debugger to be attached, PID: " << getpid() << endl;
    while (!attached) sleep(1);
}

int first_bit_set(int val) {
    for (int i = 0; i < sizeof(int) * 8; i++) {
        if (val & (1 << i))
            return i;
    }

    return -1;
}

int cancel_lsb(int val) {
    if (val == 0)
        return -1;

    return val ^ (1 << first_bit_set(c_rank));
}

void binomial_broadcast(int *value) {
    int last_node = (c_rank == 0) ? ceil(log2(c_size)) : first_bit_set(c_rank);



    if (c_rank != 0) {
        int source_rank = cancel_lsb(c_rank);
        MPI_Recv(value, 1, MPI_INT,
                source_rank, MPI_ANY_TAG, MPI_COMM_WORLD, 0);
    }

    attach_debugger(8);

    for (int i = last_node - 1; i >= 0; i--) {
        int target_rank = c_rank | (1 << i);

        if (target_rank >= c_size) continue;

        cout << c_rank << " -> " << target_rank << ";" << endl;
        MPI_Send(value, 1, MPI_INT,
                target_rank, 0, MPI_COMM_WORLD);
    }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);


    int val;

    if (c_rank == 0) {
        val = 42;
    }

    binomial_broadcast(&val);

    assert(val == 42);

    MPI_Finalize();

}
