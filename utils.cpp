#include "utils.h"

Request::Request(int timestamp, int PID)
{
    this->timestamp = timestamp;
    this->PID = PID;

}

Request::Request(Message message)
{
    this->timestamp = message.timestamp;
    this->PID = message.PID;
}

Cooldown::Cooldown(int city)
{
    this->city = city;
    this->startTime = chrono::steady_clock::now();
}

bool Cooldown::IsCooldownOver()
{
    auto now = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(now - this->startTime);
    
    return duration.count() > COOLDOWN_TIME_US;
}

Message::Message(int tab[3])
{
    this->type = tab[0];
    this->timestamp = tab[1];
    this->PID = tab[2];
}

Message::Message(int type, int timestamp, int PID)
{
    this->type = type;
    this->timestamp = timestamp;
    this->PID = PID;
}

void Send(Message message, int destination)
{
    // wysyłamy tablicę aby nie komplikować kodu, można wysyłac structy ale wymaga to tworzenia specjalnych typów (zbędna praca)
    int _message[3];
    _message[0] = message.type;
    _message[1] = message.timestamp;
    _message[2] = message.PID;

    MPI_Send(_message, 3, MPI_INT, destination, 0, MPI_COMM_WORLD);
}

void SendBroadcast(Message message, int source, int n)
{
    int _message[3];
    _message[0] = message.type;
    _message[1] = message.timestamp;
    _message[2] = message.PID;

    for (int i = 0; i < n; i++)
    {
        if (i != source)
        {
            MPI_Send(_message, 3, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
    
}

int Max(int clock, int pckClock)
{
    return clock >= pckClock ? clock : pckClock;
}


int GetIndex(vector<Request> &requestVec, Request &element)
{
    for (int i = 0; i < requestVec.size(); i++)
        if (requestVec[i] == element) return i;

    return -1;
}

int GetOldestActiveIndex(vector<Request> &requestVec, int PID)
{
    int currentBestIndex = -1;
    int currentBestTimestamp = numeric_limits<int>::max();
    for (int i = 0; i < requestVec.size(); i++)
    {
        if (requestVec[i].PID == PID && requestVec[i].isActive && requestVec[i].timestamp < currentBestTimestamp)
        {
            currentBestIndex = i;
            currentBestTimestamp = requestVec[i].timestamp;
        }
    }
    return currentBestIndex;
}

// bool Recv(Request& Request, int type, int source)
// {
//     int _message[2];

//     MPI_Request request;
//     MPI_Irecv(_message, 2, MPI_INT, source, type, MPI_COMM_WORLD, &request);

//     MPI_Status status;
//     int flag = 0;
//     MPI_Test(&request, &flag, &status);

//     if (flag)
//     {
//         Request.timestamp = _message[0];
//         Request.PID = _message[1];
//         return true;
//     }

//     return false;
// }