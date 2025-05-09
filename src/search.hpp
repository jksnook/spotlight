#pragma once

#include "movegen.hpp"
#include "eval.hpp"
#include "position.hpp"
#include "utils.hpp"
#include "tt.hpp"
#include "see.hpp"

#include <chrono>
#include <array>
#include <atomic>

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

    inline int length() {return pv_length[0];}
    inline move16 getPVMove(int ply) {return table[0][ply];}

    inline auto begin() { return table[0].begin();}
    inline auto end() { return  table[0].begin() + pv_length[0];}
private:
};

struct SearchResult {
    move16 move;
    int score;
};

class Search
{
public:
    Search(TT* _tt, std::atomic<bool>* _is_stopped);

    SearchResult timeSearch(Position &pos, int max_depth, U64 time_in_ms);
    SearchResult nodeSearch(Position &pos, int max_depth, U64 num_nodes);
    int qScore(Position &pos);
    void clearTT();
    void clearHistory();
    int tt_hits;
    int nodes_searched;
    U64 q_nodes;
    U64 total_nodes;
    bool make_output;

    int thread_id;
    std::atomic<bool>* is_stopped;

private:
    void setTimer(U64 duration_in_ms, int interval);
    template <bool pv_node, bool cut_node, bool is_root>
    int negaMax(Position& pos, int depth, int ply, int alpha, int beta);
    int qSearch(Position &pos, int depth, int ply, int alpha, int beta);
    bool timesUp();
    bool softTimesUp();
    SearchResult iterSearch(Position &pos, int max_depth);
    void outputInfo(int depth, move16 best_move, int score, int nps);
    inline void saveKiller(int ply, move16 move) {
        killer_2[ply] = killer_1[ply];
        killer_1[ply] = move;
    }
    void clearKillers();

    bool search_previous_pv;
    bool node_search;
    bool allow_nmp;
    bool times_up;
    bool enable_qsearch_tt;

    std::chrono::steady_clock::time_point start_time;
    U64 timer_duration;
    U64 soft_time_limit;

    int time_check;
    int time_check_interval;
    int max_nodes;

    move16 killer_1[MAX_PLY];
    move16 killer_2[MAX_PLY];

    int lmr_table[2][MAX_PLY][256];

    std::array<int, MAX_PLY> eval_stack;

    int quiet_history[2][64][64];
    void updateHistory(Color side, int from, int to, int bonus);

    TT* tt;
    PVTable pv;
    std::array<move16, MAX_PLY> old_pv;
    int old_pv_length;

};

} // namespace Spotlight
