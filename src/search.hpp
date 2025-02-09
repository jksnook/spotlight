#pragma once

#include "movegen.hpp"
#include "eval.hpp"
#include "position.hpp"
#include "utils.hpp"
#include "tt.hpp"
#include "moveorder.hpp"

#include <algorithm>
#include <chrono>


const int NEGATIVE_INFINITY = (1 << 31) + 1;
const int POSITIVE_INFINITY = ~0 ^ NEGATIVE_INFINITY;

class PVTable {
    public:
        std::array<std::array<move16, MAX_DEPTH>, MAX_DEPTH> table;
        std::array<move16, MAX_DEPTH> pv_length;
        void updatePV(int ply, move16 first_move);
        void updateFromTT(int ply, move16 first_move);
        void clearPV();
        void clearForNextDepth();
        void zeroLength(int ply);

        inline int length() {return pv_length[0];}
        inline move16 getPVMove(int ply) {return table[0][ply];}

        inline auto begin() { return table[0].begin();}
        inline auto end() { return  table[0].begin() + pv_length[0];}
    private:
};

class Search
{
public:
    Search();

    move16 iterSearch(Position &pos, int maxDepth, U64 time_in_ms);
    int tt_hits;
    int nodes_searched;

private:
    void setTimer(U64 duration_in_ms, int interval);
    int negaMax(Position &pos, int depth, int ply, int alpha, int beta);
    int qSearch(Position &pos, int depth, int ply, int alpha, int beta);
    bool timesUp();

    bool pv_search;
    bool times_up;

    std::chrono::steady_clock::time_point start_time;
    U64 timer_duration;

    int time_check;
    int time_check_interval;

    TT tt;
    PVTable pv;

};
