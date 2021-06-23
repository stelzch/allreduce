#include <mpi.h>
#include <iostream>
#include <cmath>
#include <cassert>
#include <cstring>
#include <bit>
#include <unistd.h>

using namespace std;

int c_rank, c_size;


void attach_debugger(int rank) {
    if (c_rank != rank) return;
    bool attached = false;

    cout << "Waiting for debugger to be attached, PID: " << getpid() << endl;
    while (!attached) sleep(1);
}

int find_first_set(int num) {
    return ffs(num) - 1;
}

int cancel_lsb(int val) {
    if (val == 0)
        return -1;

    return val ^ (1 << find_first_set(c_rank));
}

void binomial_broadcast(int *value) {
    int last_node = (c_rank == 0) ? ceil(log2(c_size)) : find_first_set(c_rank);

    /* Receive truth from predecessor */
    if (c_rank != 0) {
        int source_rank = cancel_lsb(c_rank);
        MPI::COMM_WORLD.Recv(value, 1, MPI_INT,
                source_rank, MPI_ANY_TAG);
    }

    /* Distribute it to "child" nodes */
    for (int i = last_node - 1; i >= 0; i--) {
        int target_rank = c_rank | (1 << i);

        if (target_rank >= c_size) continue;

        cout << c_rank << " -> " << target_rank << ";" << endl;
        MPI::COMM_WORLD.Send(value, 1, MPI_INT,
                target_rank, 0);
    }
}

int main(int argc, char **argv) {
    MPI::Init(argc, argv);

    c_rank = MPI::COMM_WORLD.Get_rank();
    c_size = MPI::COMM_WORLD.Get_size();

    int val;

    if (c_rank == 0) {
        // The answer to the ultimate question of life, the universe and everything
        val = 42;
    }

    binomial_broadcast(&val);

    assert(val == 42);

    MPI::Finalize();

}
