#pragma once

#include <iostream>
#include <chrono>
#include <mpi.h>
#include <vector>
#include <random>
#include <sstream>
#include <algorithm>

using namespace std;

#define REQ 10
#define ACK 20
#define REL 30

#define REST 0
#define WAIT 10
#define INSECTION 20

#define MIN_INSECTION_TIME_US 1000000
#define MAX_INSECTION_TIME_US 4000000
#define COOLDOWN_TIME_US 3000000 // czas cooldownu dla miasta w mikrosekundach

#define CITY_APPLY_PROB 0.15

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
    bool IsBetterThan(const Request& other);

    bool operator<(const Request& other) const {
        if (timestamp == other.timestamp) {
            return PID < other.PID; 
        }
        return timestamp < other.timestamp; 
    }

    bool operator==(const Request& other) const {
        if (timestamp == other.timestamp && PID == other.PID){
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

bool GetShouldApplyCity();

void PrintColor(int PID, int Clock, string text);
string AddText(string text, int City);
void ClearRequests(vector<Request> &requestVec, int m);
