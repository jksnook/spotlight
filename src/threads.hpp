#pragma once

#include "position.hpp"
#include "search.hpp"

#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

class SearchWrapper {
public:
    SearchWrapper(TT* _tt, std::atomic<bool>* _is_stopped);
    ~SearchWrapper() {};

    Search search;
    Position pos;
    std::mutex mx;
    std::condition_variable cv;

    bool node_search;
    U64 max_nodes;
    int max_depth;
    U64 time_in_ms;

    bool is_waiting;
    bool exit_thread;
    void wait();
private:
};

class Threads {
public:
    Threads(int num_threads);
    ~Threads();

    void timeSearch(Position pos, U64 time);
    void nodeSearch(Position pos, U64 nodes);
    void infiniteSearch(Position pos);
    void newGame();
    void resize(int num_threads);
    void stop();
    void finishSearch();
    void exitThreads();

    std::atomic<bool> is_stopped;
    TT tt;

private:
    std::vector<SearchWrapper*> workers;
    std::vector<std::thread> threads;
};

