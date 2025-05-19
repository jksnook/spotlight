#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <functional>

#include "eval.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "see.hpp"
#include "tt.hpp"
#include "utils.hpp"

namespace Spotlight {

const int POSITIVE_INFINITY = 32000;
const int NEGATIVE_INFINITY = -POSITIVE_INFINITY;

const int WINDOW_MIN_DEPTH = 3;
const int WINDOW_SIZE = 10;
const int WINDOW_INCREMENT = 60;
const int FUTILITY_MARGIN = 120;

class PVTable {
   public:
    std::array<std::array<move16, MAX_PLY>, MAX_PLY> table;
    std::array<int, MAX_PLY> pv_length;
    void updatePV(int ply, move16 first_move);
    void updateFromTT(int ply, move16 first_move);
    void clearPV();
    void zeroLength(int ply);

    inline int length() { return pv_length[0]; }
    inline move16 getPVMove(int ply) { return table[0][ply]; }

    inline auto begin() { return table[0].begin(); }
    inline auto end() { return table[0].begin() + pv_length[0]; }

   private:
};

struct SearchResult {
    move16 move;
    int score;
};

struct StackEntry {
    move16 move;
    Piece piece_moved;
    int s_eval;
};

class Search {
   public:
    Search(TT* _tt, std::atomic<bool>* _is_stopped, std::function<U64()> _getNodes);

    SearchResult timeSearch(Position& pos, int max_depth, U64 time_in_ms);
    SearchResult nodeSearch(Position& pos, int max_depth, U64 num_nodes);
    int qScore(Position& pos);
    void clearTT();
    void clearHistory();
    int tt_hits;
    U64 nodes_searched;
    U64 q_nodes;
    bool make_output;

    int thread_id;
    std::atomic<bool>* is_stopped;
    std::function<U64()> getNodes;

   private:
    void setTimer(U64 duration_in_ms, int interval);
    template <bool pv_node, bool cut_node, bool is_root>
    int negaMax(Position& pos, int depth, int ply, int alpha, int beta);
    int qSearch(Position& pos, int depth, int ply, int alpha, int beta);
    bool timesUp();
    bool softTimesUp();
    SearchResult iterSearch(Position& pos, int max_depth);
    void outputInfo(int depth, move16 best_move, int score);
    inline void saveKiller(int ply, move16 move) {
        killer_2[ply] = killer_1[ply];
        killer_1[ply] = move;
    }
    void clearKillers();

    bool node_search;
    bool allow_nmp;
    bool times_up;
    bool enable_qsearch_tt;

    std::chrono::steady_clock::time_point start_time;
    U64 timer_duration;
    U64 soft_time_limit;

    int time_check;
    int time_check_interval;
    U64 max_nodes;

    move16 killer_1[MAX_PLY];
    move16 killer_2[MAX_PLY];

    int lmr_table[MAX_PLY][256];

    std::array<StackEntry, MAX_PLY> search_stack;

    int quiet_history[2][64][64];
    void updateHistory(Color side, int from, int to, int bonus);

    TT* tt;
    PVTable pv;
};

}  // namespace Spotlight
