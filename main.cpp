#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>

#include "utils.h"

using namespace std;


// kopilacja: mpic++ main.cpp utils.cpp -o rozproszone
// uruchamianie: mpirun -np 4 ./rozproszone
// oba: mpic++ main.cpp utils.cpp -o rozproszone && mpirun -np 4 ./rozproszone

//operator r1 < r2 oznacza że r1 jest lepsze (wyższy priorytet) niż r2

int main(int argc, char** argv)
{
    

    int i; // identyfikator procesu
    int n, m = 10; // liczba procesów i liczba miast
    int clock = 0; // czas zegarów lamporta
    int state = REST;
    int ACK_counter = 0;
    
    int myRequestIndex = -1;
    Request myRequest(-1, -1);
    chrono::steady_clock::time_point insectionTime;

    vector<priority_queue<Request>> CityQueues(m);
    queue<Cooldown> CityCooldownQueue;
    vector<Request> RequestQueue;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &i);

    MPI_Comm_size(MPI_COMM_WORLD, &n);

    PrintColor("Started!", i);
    

    // vector potrzebny do zarządzania wiadomościami bez blokowania

    vector<MPI_Request> MPI_requests(n);

    // inicjalizujemy nasłuchiwanie
    int message_tab[3];  
    for (int p = 0; p < n; p++)
    {
        MPI_Irecv(message_tab, 3, MPI_INT, p, 0, MPI_COMM_WORLD, &MPI_requests[p]);
    }

    while (true)
    {
        queue<Message> unhandledMessages;

        for (int p = 0; p < n; p++)
        {
            int flag = 0;
            MPI_Test(&MPI_requests[i], &flag, MPI_STATUS_IGNORE);

            if (flag == 0)
            {
                Message m(message_tab);
                unhandledMessages.push(m);

                clock = Max(clock, m.timestamp);

                // przywracamy nasłuchiwanie
                MPI_Irecv(message_tab, 3, MPI_INT, i, 0, MPI_COMM_WORLD, &MPI_requests[p]);

            }
        }

        if (state == REST)
        {
            clock++;
            Message i_want_city_request_message(REQ, clock, i);
            SendBroadcast(i_want_city_request_message, i, n);
            ACK_counter = 0;

            myRequest = Request(i_want_city_request_message);
            RequestQueue.push_back(myRequest);
            std::sort(RequestQueue.begin(), RequestQueue.end());

            myRequestIndex = GetIndex(RequestQueue, myRequest);

            CityQueues[myRequestIndex % m].push(myRequest);
            
            // obsługa wiadomości
            while (!unhandledMessages.empty())
            {
                Message m = unhandledMessages.front();

                if (m.type == REQ)
                {
                    // dodajemy REQ do kolejki żądań i odsyłamy ACK
                    RequestQueue.push_back(Request(m));
                    clock++;
                    Send(Message(ACK, clock, i), m.PID);
                } 
                else if (m.type == REL)
                {
                    // zaznaczamy że najstarszy nieoznaczony REQ od adresata jest już uwolniony
                    int index = GetOldestActiveIndex(RequestQueue, m.PID);
                    RequestQueue[index].isActive = false;
                }

                unhandledMessages.pop();
            }
        } 
        else if (state == WAIT)
        {
            while (!unhandledMessages.empty())
            {
                Message m = unhandledMessages.front();
                Request incomingRequest(m);
                if (m.type == REQ)
                {
                    // przychodzące żądanie jest lepsze, odsyłamy ACK
                    if (incomingRequest < myRequest)
                    {
                        clock++;
                        Send(Message(ACK, clock, i), m.PID);
                    }
                    else
                    {
                        // zapisujemy informacje o requescie
                        RequestQueue.push_back(incomingRequest);
                    }
                } 
                else if (m.type == ACK)
                {
                    ACK_counter++;
                }
                else if (m.type == REL)
                {
                    // zaznaczamy że najstarszy nieoznaczony REQ od adresata jest już uwolniony
                    int index = GetOldestActiveIndex(RequestQueue, m.PID);
                    RequestQueue[index].isActive = false;
                }

                unhandledMessages.pop();
            }

            if (ACK_counter == n - 1)
            {
                int myCity = myRequestIndex % m;
                
                // zaczynamy od czyszczenia, nie jest to najbardziej optymalne ale na start styknie
                CityQueues = vector<priority_queue<Request>>(m);

                // budowanie kolejek, przeglądamy każdy element posortowanej tablicy aż do naszego
                for (int i = 0; i < myRequestIndex; i++)
                {
                    int City = i % m;
                    if (RequestQueue[i].isActive)
                    {
                        CityQueues[City].push(RequestQueue[i]);
                    }
                }

                // nasze żądanie jest na szczycie kolejki miasta
                if (CityQueues[myCity].top() == myRequest)
                {
                    state = INSECTION; 
                    insectionTime = GetRandomInsectionTime();
                }

            }

        }
        else if (state == INSECTION) 
        {
            while (!unhandledMessages.empty())
            {
                Message m = unhandledMessages.front();
                Request incomingRequest(m);
                
                if (m.type == REQ)
                {
                    // dodaje REQ do swojej kolejki żądań i odsyła ACK
                    RequestQueue.push_back(incomingRequest);
                    clock++;
                    Send(Message(ACK, clock, i), m.PID);
                } 
                else if (m.type == REL)
                {
                    // zaznaczamy że najstarszy nieoznaczony REQ od adresata jest już uwolniony
                    int index = GetOldestActiveIndex(RequestQueue, m.PID);
                    RequestQueue[index].isActive = false;
                }

                unhandledMessages.pop();
            }

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= insectionTime)
            {   
                // dodajemy do kolejki cooldownu
                CityCooldownQueue.push(Cooldown(myRequestIndex));
          
                state = REST;
            }
        }

        // przeglądanie kolejki cooldownu
        if (!CityCooldownQueue.empty())
        {
            Cooldown CurrentCity = CityCooldownQueue.front();
            while (CurrentCity.IsCooldownOver())
            {
                int oldestIndex = GetOldestActiveIndex(RequestQueue, i);
                RequestQueue[oldestIndex].isActive = false;

                clock++;
                SendBroadcast(Message(REL, clock, i), i, n);

                CityCooldownQueue.pop();

                if (CityCooldownQueue.empty()) break;

                CurrentCity = CityCooldownQueue.front();
            }
        }

    }

    MPI_Finalize();

    return 0;
}