#pragma once

#include "position.hpp"
#include "search.hpp"

#include <thread>
#include <vector>
#include <atomic>

class SearchWrapper {
public:
    SearchWrapper(TT* tt, std::atomic_bool* is_stopped);
    // SearchWrapper(TT* tt, std::atomic_bool* is_stopped, Position _pos);

    // Position pos;
    Search search;
private:
};

class Threads {
public:
    Threads(int num_threads);

    void go(Position pos, U64 time);
    void newGame();


    std::vector<SearchWrapper> workers;
    std::vector<std::thread> threads;

    std::atomic<bool> is_stopped;

    TT tt;

private:

};

