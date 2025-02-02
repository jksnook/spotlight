#pragma once

#include "movegen.hpp"
#include "eval.hpp"
#include "position.hpp"
#include "utils.hpp"

#include <algorithm>
#include <chrono>

const int NEGATIVE_INFINITY = (1 << 31) + 1;
const int POSITIVE_INFINITY = ~0 ^ NEGATIVE_INFINITY;
const int MATE_SCORE = 1 << 15;

class Search
{
public:
    Search();

    move16 iterSearch(Position &pos, int maxDepth, U64 time_in_ms);

private:
    void setTimer(U64 duration_in_ms, int interval);
    int negaMax(Position &pos, int depth, int alpha, int beta);
    int qSearch(Position &pos, int depth, int alpha, int beta);
    bool timesUp();

    bool times_up;

    std::chrono::steady_clock::time_point start_time;
    U64 timer_duration;

    int time_check;
    int time_check_interval;

};
