#include <mpi.h>
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>

#include "utils.h"

//mpic++ tester.cpp utils.cpp -o test && mpirun -np 1 ./test

int main()
{
    // test operatorów
    vector<Request> RequestQueue;
    RequestQueue.push_back(Request(1, 2));
    RequestQueue.push_back(Request(0, 1));
    RequestQueue.push_back(Request(4, 5));
    RequestQueue.push_back(Request(2, 4));
    RequestQueue.push_back(Request(1, 3));

    std::sort(RequestQueue.begin(), RequestQueue.end());
    for (int i = 0; i < RequestQueue.size(); i++)
    {
        cout << RequestQueue[i].timestamp << " " << RequestQueue[i].PID << endl;
    }

    Request T1(0, 1);
    Request T2(0, 2);
    if (T1.IsBetterThan(T2))
        cout << "correct" << endl;
    else 
        cout << "incorrect" << endl;

    Request T3(1, 1);
    Request T4(0, 2);
    if (T3.IsBetterThan(T4))
        cout << "incorrect" << endl;
    else 
        cout << "correct" << endl;

    return 0;
}