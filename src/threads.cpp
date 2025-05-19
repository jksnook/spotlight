#include "threads.hpp"

#include "search.hpp"

namespace Spotlight {

SearchWrapper::SearchWrapper(TT* _tt, std::atomic<bool>* _is_stopped,
                             std::function<U64()> _getNodes)
    : search(_tt, _is_stopped, _getNodes),
      pos(),
      node_search(false),
      max_nodes(0ULL),
      max_depth(MAX_PLY),
      time_in_ms(0ULL),
      is_waiting(true),
      exit_thread(false) {}

void SearchWrapper::wait() {
    while (true) {
        std::unique_lock lock(mx);
        cv.wait(lock, [&] { return !is_waiting; });

        is_waiting = true;

        if (exit_thread) break;

        if (node_search) {
            search.nodeSearch(pos, max_depth, max_nodes);
        } else {
            search.timeSearch(pos, max_depth, time_in_ms);
        }

        cv.notify_all();
    }
}

Threads::Threads(int num_threads) : is_stopped(true), tt() { resize(num_threads); }

Threads::~Threads() {
    exitThreads();

    for (int i = 0; i < static_cast<int>(workers.size()); i++) {
        delete workers[i];
    }
}

void Threads::timeSearch(Position pos, U64 time) {
    is_stopped.store(false);

    for (int i = 0; i < static_cast<int>(workers.size()); i++) {
        std::lock_guard lock(workers[i]->mx);
        workers[i]->pos = pos;
        workers[i]->node_search = false;
        workers[i]->max_nodes = 0;
        workers[i]->max_depth = MAX_PLY;
        workers[i]->time_in_ms = time;
        workers[i]->is_waiting = false;
        workers[i]->cv.notify_all();
    }
}

// Currently only counts nodes locally per thread
void Threads::nodeSearch(Position pos, U64 nodes) {
    is_stopped.store(false);

    for (int i = 0; i <static_cast<int>(workers.size()); i++) {
        std::lock_guard lock(workers[i]->mx);
        workers[i]->pos = pos;
        workers[i]->node_search = true;
        workers[i]->max_nodes = nodes;
        workers[i]->max_depth = MAX_PLY;
        workers[i]->is_waiting = false;
        workers[i]->cv.notify_all();
    }
}

void Threads::infiniteSearch(Position pos) { timeSearch(pos, 999999999); }

void Threads::newGame() {
    stop();
    tt.clear();

    for (int i = 0; i < static_cast<int>(workers.size()); i++) {
        workers[i]->search.clearHistory();
    }
}

void Threads::resize(int num_threads) {
    exitThreads();
    for (int i = 0; i < static_cast<int>(workers.size()); i++) {
        delete workers[i];
    }
    workers.clear();
    threads.clear();

    for (int i = 0; i < num_threads; i++) {
        workers.emplace_back(new SearchWrapper(&tt, &is_stopped, [this]() { return getNodes(); }));
        workers[i]->search.thread_id = i;
        threads.emplace_back(std::thread([this, i] { workers[i]->wait(); }));
    }

    // std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

void Threads::stop() {
    is_stopped.store(true);

    for (int i = 0; i < static_cast<int>(workers.size()); i++) {
        std::unique_lock lock(workers[i]->mx);
        workers[i]->cv.wait(lock, [&] { return workers[i]->is_waiting; });
    }
}

void Threads::finishSearch() {
    for (int i = 0; i < static_cast<int>(workers.size()); i++) {
        std::unique_lock lock(workers[i]->mx);
        workers[i]->cv.wait(lock, [&] { return workers[i]->is_waiting; });
    }
}

void Threads::exitThreads() {
    is_stopped.store(true);

    for (int i = 0; i < static_cast<int>(workers.size()); i++) {
        std::lock_guard lock(workers[i]->mx);
        workers[i]->exit_thread = true;
        workers[i]->is_waiting = false;
        workers[i]->cv.notify_all();
    }

    for (int i = 0; i < static_cast<int>(threads.size()); i++) {
        if (threads[i].joinable()) threads[i].join();
    }
}

U64 Threads::getNodes() {
    U64 nodes = 0ULL;
    for (const auto& w : workers) {
        nodes += w->search.nodes_searched;
    }
    return nodes;
}

}  // namespace Spotlight
