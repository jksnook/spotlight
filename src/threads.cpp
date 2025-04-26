#include "threads.hpp"
#include "search.hpp"

Threads::Threads(int num_threads): tt(), is_stopped(false) {

    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(Search(&tt, &is_stopped));
        workers[i].thread_id = i;
    }

}

void Threads::timeSearch(Position pos, U64 time) {
    threads.clear();
    is_stopped.store(false);
    
    for (int i = 0; i < workers.size(); i++) {
        threads.emplace_back(std::thread([this, i](Position p, int d, U64 t) {workers[i].timeSearch(p, d, t);}, pos, MAX_PLY, time));
    }

    // for (int i = 0; i < threads.size(); i++) {
    //     if(threads[i].joinable()) threads[i].join();
    // }

}

void Threads::nodeSearch(Position pos, U64 nodes) {
    threads.clear();
    is_stopped.store(false);
    
    for (int i = 0; i < workers.size(); i++) {
        threads.emplace_back(std::thread([this, i](Position p, int d, U64 n) {workers[i].nodeSearch(p, d, n);}, pos, MAX_PLY, nodes));
    }

}

void Threads::infiniteSearch(Position pos) {
    threads.clear();
    is_stopped.store(false);
}

void Threads::newGame() {
    stop();
    tt.clear();

    for (int i = 0; i < workers.size(); i++) {
        workers[i].clearHistory();
    }
}

void Threads::resize(int num_threads) {
    stop();
    workers.clear();
    threads.clear();
    // tt.clear();
    is_stopped.store(false);

    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(Search(&tt, &is_stopped));
        workers[i].thread_id = i;
    }
}

void Threads::stop() {
    is_stopped.store(true);

    for (int i = 0; i < threads.size(); i++) {
        if(threads[i].joinable()) threads[i].join();
    }
}