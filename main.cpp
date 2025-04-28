#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>

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
    
    int myCity = -1;
    Request myRequest(-1, -1);

    vector<priority_queue<Request>> CityQueues(m);
    queue<Cooldown> CityCooldownQueue;
    vector<Request> RequestQueue;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &i);

    MPI_Comm_size(MPI_COMM_WORLD, &n);

    

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
            Message i_want_city_request(REQ, clock, i);
            SendBroadcast(i_want_city_request, i, n);
            ACK_counter = 0;

            myRequest = Request(i_want_city_request);
            RequestQueue.push_back(Request(myRequest));
            std::sort(RequestQueue.begin(), RequestQueue.end());

            myCity = GetIndex(RequestQueue, myRequest) % m;
            

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
            if (ACK_counter == n - 1)
            {
                std::sort(RequestQueue.begin(), RequestQueue.end());
                myCity = GetOldestActiveIndex(RequestQueue, i) % m;

                // TODO budowanie kolejek

                state = INSECTION; 
            }


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

                }

                unhandledMessages.pop();
            }
        }
        else if (state == INSECTION) 
        {

            while (!unhandledMessages.empty())
            {
                Message m = unhandledMessages.front();
                
                if (m.type == REQ)
                {

                } else if (m.type == REL)
                {

                }

                unhandledMessages.pop();
            }
        }

        

    }

    MPI_Finalize();

    return 0;
}