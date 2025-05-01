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
    this->endTime = GetCooldownTime();
}

bool Cooldown::IsCooldownOver()
{
    auto now = chrono::steady_clock::now();
    
    return now > endTime;
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

chrono::steady_clock::time_point GetRandomInsectionTime()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(MIN_INSECTION_TIME_US, MAX_INSECTION_TIME_US);

    int delayUs = dist(gen);

    auto now = std::chrono::steady_clock::now();

    auto delayed_time = now + std::chrono::microseconds(delayUs);

    return delayed_time;
}


chrono::steady_clock::time_point GetCooldownTime()
{
    return std::chrono::steady_clock::now() + std::chrono::microseconds(COOLDOWN_TIME_US);
}


void PrintColor(string text, int PID) {
    const char* colors[] = {
        "\x1B[31m", // Czerwony
        "\x1B[32m", // Zielony
        "\x1B[33m", // Żółty
        "\x1B[34m", // Niebieski
        "\x1B[35m", // Fioletowy
        "\x1B[36m", // Cyjan
        "\x1B[37m", // Biały
    };

    // Wybór koloru na podstawie pid
    const char* color = colors[PID % 7]; // Modulo, aby nie wyjść poza tablicę

    // Wypisanie wiadomości w kolorze
    cout << color << "[" << PID << "] " << text << "\033[0m" << endl; 
}