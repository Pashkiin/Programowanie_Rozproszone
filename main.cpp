#include <mpi.h>
#include <iostream>
#include "observer.h"
#include "thief.h"

int main(int argc, char** argv) {
    if (argc != 2)
    {
        std::cout << "Podano zbyt mala ilosc parametrow" << std::endl;
        return 0;
    }
    
    MPI_Init(&argc, &argv);

    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int world_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (world_rank == 0) {
        // Proces Obserwatora
        int house_num = std::stoi(argv[1]);
        observer(house_num);
    } else {
        // Procesy Złodziei
        int house_num = std::stoi(argv[1]);
        thief(world_rank, world_size, house_num);
    }

    MPI_Finalize();
    return 0;
}
