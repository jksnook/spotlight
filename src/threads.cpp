#include "threads.hpp"
#include "search.hpp"

SearchWrapper::SearchWrapper(TT* tt, std::atomic_bool* is_stopped): search(tt, is_stopped) {
}


// SearchWrapper::SearchWrapper(TT* tt, std::atomic_bool* is_stopped, Position _pos): search(tt, is_stopped), pos(_pos) {
// }

Threads::Threads(int num_threads): tt(), is_stopped(false) {

    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(SearchWrapper(&tt, &is_stopped));
        workers[i].search.thread_id = i;
    }

}

void Threads::go(Position pos, U64 time) {
    threads.clear();

    // SearchWrapper search_wrapper(,pos);
    is_stopped.store(false);
    
    for (int i = 0; i < workers.size(); i++) {
        threads.emplace_back(std::thread([this, i](Position p, int d, U64 t) {workers[i].search.timeSearch(p, d, t);}, pos, MAX_PLY, time));
    }

    for (int i = 0; i < workers.size(); i++) {
        threads[i].join();
    }
}

void Threads::newGame() {
    tt.clear();

    for (int i = 0; i < workers.size(); i++) {
        workers[i].search.clearHistory();
    }
}