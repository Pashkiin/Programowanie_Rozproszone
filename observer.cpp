#include <mpi.h>
#include <iostream>
#include <thread>
#include <chrono>
#include "observer.h"

void observer(int house_num) {
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int new_house = 1; // Numer nowego domu do obdarowania
    int houses_in_progress = 0; // Liczba domów w trakcie obdarowania
    while (true) {
        // Powiadomienie złodziei o nowym domu/domach
        std::cout << "Obserwator: Nowy dom " << new_house << std::endl;
        for (int i = 1; i < world_size; i++) {
            MPI_Send(&new_house, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
        std::cout << "Obserwator: Wysłałem informację o nowym domu " << new_house << std::endl;
        new_house++;
        houses_in_progress++;
        // Odbieranie potwierdzenia od złodzieja
        if(houses_in_progress == house_num){
            int confirmation;
            MPI_Status status;
            MPI_Recv(&confirmation, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            std::cout << "Obserwator: Złodziej " << status.MPI_SOURCE << " obdarował dom " << confirmation<< std::endl;
            houses_in_progress--;
        }
        
        // Opóźnienie lub warunek zakończenia, aby symulować rzeczywiste działanie
        //std::this_thread::sleep_for(std::chrono::seconds(3));
    }
}
