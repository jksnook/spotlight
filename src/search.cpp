#include "search.hpp"

Search::Search(): start_time(std::chrono::steady_clock::now()) {

}

void Search::setTimer(U64 duration_in_ms, int interval) {
    time_check_interval = interval;
    timer_duration = duration_in_ms;
    time_check = interval;
    times_up = false;
    start_time = std::chrono::steady_clock::now();
}

bool Search::timesUp() {
    if (times_up) {
        return true;
    } else if (time_check > 0) {
        time_check--;
        return false;
    }
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    if (time_elapsed.count() > timer_duration) {
        times_up = true;
        return true;
    }
    time_check = time_check_interval;
    return false;
}

move16 Search::iterSearch(Position &pos, int max_depth, U64 time_in_ms) {
    setTimer(time_in_ms, 1000);

    MoveList moves;

    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    move16 best_move = 0;
    int max_score = NEGATIVE_INFINITY;

    for (int depth = 0; depth < max_depth; depth++) {
        int beta = POSITIVE_INFINITY;
        int alpha = NEGATIVE_INFINITY;
        for (const auto &move: moves) {
            pos.makeMove(move);
            int score = -negaMax(pos, depth, -beta, -alpha);
            pos.unmakeMove();
            if (score > alpha) {
                if (timesUp()) {
                    break;
                }
                alpha = score;
                if (score > max_score) {
                    best_move = move;
                    max_score = score;
                }
            }
        }
        std::cout << "info depth " << depth + 1 << " bestmove " << moveToString(best_move) << std::endl;
        if (timesUp()) {
            break;
        }
    }

    return best_move;
}

int Search::negaMax(Position &pos, int depth, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    }

    if (depth == 0) {
        return eval(pos);
    }

    MoveList moves;
    
    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    // check for stalemate or checkmate
    if (moves.size() == 0) {
        // if in check return negative infinity else return 0
        if (pos.in_check) {
            return NEGATIVE_INFINITY + depth;
        }
        return 0;
    }
    
    int max_score = NEGATIVE_INFINITY;

    for (const auto &move: moves) {
        pos.makeMove(move);
        // int score = -negaMax(pos, depth - 1, beta, alpha);
        max_score = std::max(max_score, -negaMax(pos, depth - 1, -beta, -alpha));
        alpha = std::max(alpha, max_score);
        pos.unmakeMove();
        if (max_score >= beta) {
            return max_score;
        }
    }

    return max_score;
};