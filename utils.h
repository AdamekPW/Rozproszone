#pragma once

#include <iostream>
#include <chrono>
#include <mpi.h>
#include <vector>
#include <random>

using namespace std;

#define REQ 1
#define ACK 2
#define REL 3

#define REST 0
#define WAIT 1
#define INSECTION 2

#define MIN_INSECTION_TIME_US 10000000
#define MAX_INSECTION_TIME_US 40000000
#define COOLDOWN_TIME_US 10000000 // czas cooldownu dla miasta w mikrosekundach


struct Message 
{
    int type;
    int timestamp;
    int PID;

    Message(int tab[3]);
    Message(int type, int timestamp, int PID);
};


struct Request
{
    int timestamp;
    int PID;
    bool isActive = true;

    Request(int timestamp, int PID);
    Request(Message message);
    bool operator<(const Request& other) const {
        if (timestamp == other.timestamp) {
            return PID < other.PID; // W przypadku remisu, mniejszy PID ma wyższy priorytet
        }
        return timestamp < other.timestamp; // Mniejszy timestamp ma wyższy priorytet
    }

    bool operator==(const Request& other) const {
        if (timestamp == other.timestamp && PID == other.PID && isActive == other.isActive){
            return true;
        }
        return false;
    }

};



struct Cooldown
{
    int city;
    chrono::steady_clock::time_point endTime;

    Cooldown(int city);
    bool IsCooldownOver();
};

void Send(Message message, int destination);
void SendBroadcast(Message message, int source, int n);

int Max(int clock, int pckClock);

int GetIndex(vector<Request> &requestVec, Request &element);
int GetOldestActiveIndex(vector<Request> &requestVec, int PID);

chrono::steady_clock::time_point GetRandomInsectionTime();
chrono::steady_clock::time_point GetCooldownTime();

void PrintColor(string text, int PID);

