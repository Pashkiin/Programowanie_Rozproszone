#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <thread>
#include <chrono>
#include "thief.h"

struct Request {
    int timestamp;
    int process_id;
    
    bool operator<(const Request& other) const {
        return (timestamp == other.timestamp) ? process_id < other.process_id : timestamp > other.timestamp;
    }
};

std::vector<int> lamport_clocks;
std::priority_queue<Request> request_queue;
std::priority_queue<int> ready_houses;
int replies = 0;
int houses_to_pop = 0;

int minimal(int a, int b, int c) {
    return std::min(a, std::min(b, c));
}

void print_state(int rank) {
    std::cout << "Złodziej " << rank << ": Kolejka żądań: ";
    std::priority_queue<Request> temp = request_queue;
    while (!temp.empty()) {
        std::cout << "(zegar:" << temp.top().timestamp << ", id: " << temp.top().process_id << ") ";
        temp.pop();
    }
    std::cout << std::endl;
}

// Funkcja sprawdzajaca czy pozycja w kolejce oraz ilosc dostepnych 
// domow pozawala na wejscie do sekcji krytycznej
bool process_requests(int rank, int house_num) {
    if (ready_houses.empty()) return false;
    print_state(rank);
    std::priority_queue<Request> tmp_queue = request_queue;
    int size = tmp_queue.size();
    int var = minimal(size, house_num, ready_houses.size());

    for (int i = 0; i < var; i++) {
        if (!tmp_queue.empty() && tmp_queue.top().process_id == rank) {
            return true;
        } else {
            tmp_queue.pop();
        }
    }

    return false;
}

bool process_reply(int rank, int house_num) {
    MPI_Status status;
    int received_clock;
    std::cout << "Złodziej " << rank << ": Oczekuję na nowa wiadomosc" << std::endl;
    MPI_Recv(&received_clock, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    int sender = status.MPI_SOURCE;

    // zaktualizowanie zegarów
    lamport_clocks[rank] = std::max(lamport_clocks[rank], received_clock) + 1;
    lamport_clocks[sender] = received_clock;

    switch (status.MPI_TAG) {
        case 2:
        //handle ack
            replies++;
            break;
        case 1:
        //handle request
            std::cout << "Złodziej " << rank << ": Otrzymałem REQ od złodzieja " << sender << std::endl;
            request_queue.push({received_clock, sender});
            print_state(rank);
            std::cout << "Złodziej " << rank << ": Wysłałem ACK do złodzieja " << sender << std::endl;
            MPI_Send(&lamport_clocks[rank], 1, MPI_INT, sender, 2, MPI_COMM_WORLD);
            break;
        case 3:
        //handle release
            request_queue.pop();
            if(!ready_houses.empty()) ready_houses.pop();
            else houses_to_pop++;
            //jesli jestem uprawniony oraz sa domy do obdarowania to wchodze do sekcji krytycznej
            return process_requests(rank, house_num);
        case 0:
        //handle new house
            if(houses_to_pop>0) houses_to_pop--;
            else ready_houses.push(received_clock);
        
            std::cout << "Złodziej " << rank << ": Otrzymałem informację o nowym domu " << received_clock << std::endl;
            //jesli jestem uprawniony oraz sa domy do obdarowania to wchodze do sekcji krytycznej
            return process_requests(rank, house_num);
    }
    return false;
}

void initialize_requests(int rank, int world_size) {
    lamport_clocks[rank]++;
    Request req = {lamport_clocks[rank], rank};
    request_queue.push(req);
    
    for (int i = 1; i < world_size; i++) {
        if (i != rank) {
            std::cout << "Złodziej " << rank << ": Wysyłam REQ do złodzieja " << i << std::endl;
            MPI_Send(&lamport_clocks[rank], 1, MPI_INT, i, 1, MPI_COMM_WORLD);
        }
    }

    int replies = 0;
    while (replies < world_size - 2) {
        int received_clock;
        MPI_Status status;
        MPI_Recv(&received_clock, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
        std::cout << "Złodziej " << rank << ": Otrzymałem REQ od złodzieja " << status.MPI_SOURCE << std::endl;
        lamport_clocks[rank] = std::max(lamport_clocks[rank], received_clock);
        lamport_clocks[status.MPI_SOURCE] = received_clock;
        request_queue.push({received_clock, status.MPI_SOURCE});
        replies++;
    }
    lamport_clocks[rank]++;
}

void critical_section(int rank, int world_size) {
    int house = ready_houses.top();
    request_queue.pop();
    ready_houses.pop();
    std::cout << "Złodziej " << rank << ": Obdarowuję dom " << house << std::endl;
    //std::this_thread::sleep_for(std::chrono::seconds(4));
    lamport_clocks[rank]++;
    std::cout << "Złodziej " << rank << ": Wysyłam informację o obdarowanym domu " << house << " do Obserwatora" << std::endl;
    MPI_Send(&house, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);

    for (int i = 1; i < world_size; i++) {
        if (i != rank) {
            MPI_Send(&house, 1, MPI_INT, i, 3, MPI_COMM_WORLD);
        }
    }
}

void send_request(int rank, int world_size) {
    lamport_clocks[rank]++;
    Request req = {lamport_clocks[rank], rank};
    request_queue.push(req);
    replies = 0;
    for (int i = 1; i < world_size; i++) {
        if (i != rank) {
            std::cout << "Złodziej " << rank << ": Wysyłam REQ do złodzieja " << i << std::endl;
            MPI_Send(&lamport_clocks[rank], 1, MPI_INT, i, 1, MPI_COMM_WORLD);
        }
    }

    while(replies < world_size-2){
        bool result = process_reply(rank, 0);
    }
    std::cout << "Złodziej " << rank << ": Otrzymałem  ACK od wszystkich złodziei" << std::endl;
}

void thief(int rank, int world_size, int house_num) {
    int new_house;
    lamport_clocks.resize(world_size, 0);
    request_queue = std::priority_queue<Request>();

    initialize_requests(rank, world_size);
    goto beginning;
    while (true) {
        send_request(rank, world_size);
        beginning:
        bool result = process_requests(rank, house_num);
        while (true) {
            if (result) {
                critical_section(rank, world_size);
                break;
            }
            result = process_reply(rank, house_num);
        }
    }
}
