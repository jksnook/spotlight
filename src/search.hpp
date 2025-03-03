#pragma once

#include "movegen.hpp"
#include "eval.hpp"
#include "position.hpp"
#include "utils.hpp"
#include "tt.hpp"
#include "moveorder.hpp"

#include <chrono>


const int NEGATIVE_INFINITY = (1 << 31) + 1;
const int POSITIVE_INFINITY = ~0 ^ NEGATIVE_INFINITY;

const int WINDOW_SIZE = 60;
const int NMP_REDUCTION = 4;

class PVTable {
    public:
        std::array<std::array<move16, MAX_DEPTH>, MAX_DEPTH> table;
        std::array<move16, MAX_DEPTH> pv_length;
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
    Search();

    SearchResult iterSearch(Position &pos, int max_depth, U64 time_in_ms);
    SearchResult nodeSearch(Position &pos, int max_depth, U64 num_nodes);
    void clearTT();
    int tt_hits;
    int nodes_searched;

private:
    void setTimer(U64 duration_in_ms, int interval);
    int negaMax(Position &pos, int depth, int ply, int alpha, int beta);
    int qSearch(Position &pos, int depth, int ply, int alpha, int beta);
    bool timesUp();
    SearchResult rootSearch(Position &pos, MoveList &moves, int depth, int alpha, int beta);
    void outputInfo(int depth, move16 best_move, int score, int nps);
    inline void saveKiller(int ply, move16 move) {
        killer_2[ply] = killer_1[ply];
        killer_1[ply] = move;
    }

    bool pv_search;
    bool node_search;
    bool allow_nmp;
    bool times_up;

    std::chrono::steady_clock::time_point start_time;
    U64 timer_duration;

    int time_check;
    int time_check_interval;
    int max_nodes;

    move16 killer_1[MAX_DEPTH];
    move16 killer_2[MAX_DEPTH];

    TT tt;
    PVTable pv;

};
