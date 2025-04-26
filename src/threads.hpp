#pragma once

#include "position.hpp"
#include "search.hpp"

#include <thread>
#include <vector>
#include <atomic>


class Threads {
public:
    Threads(int num_threads);

    void timeSearch(Position pos, U64 time);
    void nodeSearch(Position pos, U64 nodes);
    void infiniteSearch(Position pos);
    void newGame();
    void resize(int num_threads);
    void stop();

private:
    std::vector<Search> workers;
    std::vector<std::thread> threads;
    std::atomic<bool> is_stopped;
    
    TT tt;
};

