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

    bool times_up;

    std::chrono::steady_clock::time_point start_time;
    U64 timer_duration;

    int time_check;
    int time_check_interval;

    TT tt;

};
