#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>
#include <thread>
#include <array>

#include "utils.h"

using namespace std;

//#define DEBUG_REST
//#define DEBUG_WAIT
//#define DEBUT_INSECTION

#define GARBAGE_COLLECTOR

// kopilacja: mpic++ main.cpp utils.cpp -o rozproszone
// uruchamianie: mpirun -np 4 ./rozproszone
// oba: mpic++ main.cpp utils.cpp -o rozproszone && mpirun -np 4 ./rozproszone


int main(int argc, char** argv)
{
    

    int i; // identyfikator procesu
    int n, m = 2; // liczba procesów i liczba miast
    int clock = 0; // czas zegarów lamporta
    int state = REST;
    int ACK_counter = 0;
    
    int myRequestIndex = -1;
    Request myRequest(-1, -1);
    chrono::steady_clock::time_point insectionTime;

    queue<Cooldown> CityCooldownQueue;
    vector<Request> RequestQueue;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &i);

    MPI_Comm_size(MPI_COMM_WORLD, &n);


    // inicjalizujemy nasłuchiwanie
    std::vector<std::array<int, 3>> message_buffers(n);  // oddzielny bufor dla każdego procesu
    std::vector<MPI_Request> MPI_requests(n);
    
    // Inicjalizacja odbioru
    for (int p = 0; p < n; ++p) {
        MPI_Irecv(message_buffers[p].data(), 3, MPI_INT, p, 0, MPI_COMM_WORLD, &MPI_requests[p]);
    }
    
    while (true) {
        queue<Message> unhandledMessages;
    
        // odbieranie wiadomości i i wrzucanie do kolejki unhandledMessages
        // dzięki temu w każdym stanie możemy przetworzyć wiadomość inaczej
        for (int p = 0; p < n; ++p) {
            int flag = 0;
            MPI_Test(&MPI_requests[p], &flag, MPI_STATUS_IGNORE);
    
            if (flag != 0) {
                Message mes(message_buffers[p].data());
                unhandledMessages.push(mes);
    
                clock = Max(clock, mes.timestamp);
    
                // Odbieraj dalej od tego samego źródła
                MPI_Irecv(message_buffers[p].data(), 3, MPI_INT, p, 0, MPI_COMM_WORLD, &MPI_requests[p]);
            }
        
        }
 
        
        if (state == REST)
        {
            #ifdef DEBUG_REST
            PrintColor(i, clock, "R1");
            #endif

            //TUTAJ LOSUJE CZY CHCE MIASTO
            // TRUE/FALSE
            bool do_apply = GetShouldApplyCity();
            if(do_apply)
            {
                clock++;
                Message i_want_city_request_message(REQ, clock, i);
                SendBroadcast(i_want_city_request_message, i, n);
                ACK_counter = 0;
    
                myRequest = Request(i_want_city_request_message);
                RequestQueue.push_back(myRequest);
            }

            
            #ifdef DEBUG_REST
            PrintColor(i, clock, "R2");
            #endif
            //
            // obsługa wiadomości
            while (!unhandledMessages.empty())
            {
                Message mes = unhandledMessages.front();

                if (mes.type == REQ)
                {
                    // dodajemy REQ do kolejki żądań i odsyłamy ACK
                    RequestQueue.push_back(Request(mes));
                    clock++;
                    Send(Message(ACK, clock, i), mes.PID);
                } 
                else if (mes.type == REL)
                {
                    // zaznaczamy że najstarszy nieoznaczony REQ od adresata jest już uwolniony
                    int index = GetOldestActiveIndex(RequestQueue, mes.PID);
                    RequestQueue[index].isActive = false;
                }

                unhandledMessages.pop();
            }

            if(do_apply)
            {
                state = WAIT;
                PrintColor(i, clock, "Wchodze do stanu WAIT, zbieram ACK");
            }

            #ifdef DEBUG_REST
            PrintColor(i, clock, "R3");
            #endif
        } 

        else if (state == WAIT)
        {
            
            while (!unhandledMessages.empty())
            {
                #ifdef DEBUG_WAIT
                PrintColor(i, clock, "W1");
                #endif

                Message mes = unhandledMessages.front();
                Request incomingRequest(mes);
                if (mes.type == REQ)
                {
                    //PrintColor(i, clock, AddText("Otrzymano wiadomość od: ", mes.PID));
                    // przychodzące żądanie jest lepsze, odsyłamy ACK
                    RequestQueue.push_back(incomingRequest);

                    


                    clock++;
                    Send(Message(ACK, clock, i), mes.PID);
                   
                } 
                else if (mes.type == ACK)
                {
                    ACK_counter++;
                }
                else if (mes.type == REL)
                {
                    // zaznaczamy że najstarszy nieoznaczony REQ od adresata jest już uwolniony
                    int index = GetOldestActiveIndex(RequestQueue, mes.PID);
                    RequestQueue[index].isActive = false;
                }

                unhandledMessages.pop();
            }

            
            if (ACK_counter == n - 1)
            {
                #ifdef DEBUG_WAIT
                PrintColor(i, clock, "W2");
                #endif

                //PrintColor(i, clock, "Uzbierano wszystkie ACK!");

                std::sort(RequestQueue.begin(), RequestQueue.end());

                myRequestIndex = GetIndex(RequestQueue, myRequest);
                
                int myCity = myRequestIndex % m;
            
                Request bestCityRequest(-1, -1);

                int older_index = myRequestIndex - m;
                if (myRequestIndex < m || (myRequestIndex >= m && !RequestQueue[older_index].isActive)) {
                    state = INSECTION; 
                    insectionTime = GetRandomInsectionTime();
                    PrintColor(i, clock, AddText("Zajmuje", myCity));
                }

                #ifdef DEBUG_WAIT
                PrintColor(i, clock, "W3");
                #endif
            }
        }

        else if (state == INSECTION) 
        {
            while (!unhandledMessages.empty())
            {
                Message mes = unhandledMessages.front();
                Request incomingRequest(mes);
                
                if (mes.type == REQ)
                {
                    // dodaje REQ do swojej kolejki żądań i odsyła ACK
                    RequestQueue.push_back(incomingRequest);
                    clock++;
                    Send(Message(ACK, clock, i), mes.PID);
                } 
                else if (mes.type == REL)
                {
                    // zaznaczamy że najstarszy nieoznaczony REQ od adresata jest już uwolniony
                    int index = GetOldestActiveIndex(RequestQueue, mes.PID);
                    RequestQueue[index].isActive = false;
                }

                unhandledMessages.pop();
            }

            auto currentTime = std::chrono::steady_clock::now();
            if (currentTime >= insectionTime)
            {   
                // dodajemy do kolejki cooldownu
                PrintColor(i, clock, AddText("Dodaje do kolejki cooldownu miasto ", myRequestIndex % m));
                CityCooldownQueue.push(Cooldown(myRequestIndex % m));
          
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

                PrintColor(i, clock, AddText("Zwalniam", oldestIndex % m));

                clock++;
                SendBroadcast(Message(REL, clock, i), i, n);

                CityCooldownQueue.pop();

                if (CityCooldownQueue.empty()) break;

                CurrentCity = CityCooldownQueue.front();
            }
        }

        //usuwanie śmieci
        #ifdef GARBAGE_COLLECTOR
        ClearRequests(RequestQueue, m);
        #endif
    }

    MPI_Finalize();

    return 0;
}