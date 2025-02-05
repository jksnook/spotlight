#include "search.hpp"

Search::Search(): start_time(std::chrono::steady_clock::now()), tt_hits(0) {

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
    tt_hits = 0;

    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    move16 best_move = 0;
    int max_score = NEGATIVE_INFINITY;

    for (int depth = 0; depth < max_depth; depth++) {
        nodes_searched = 1;
        orderMoves(pos, moves, best_move);
        int beta = POSITIVE_INFINITY;
        int alpha = NEGATIVE_INFINITY;
        move16 best_move_this_search = 0;
        for (const auto &move: moves) {
            pos.makeMove(move);
            int score = -negaMax(pos, depth, 1, -beta, -alpha);
            pos.unmakeMove();
            if (score > alpha) {
                if (timesUp()) {
                    break;
                }
                alpha = score;
                best_move_this_search = move;
                if (score > max_score) {
                    best_move = move;
                    max_score = score;
                }
            }
        }
        std::cout << "info depth " << depth + 1 << " nodes " << nodes_searched;
        if (timesUp()) {
            std::cout << " bestmove " << moveToString(best_move) << " score " << max_score << std::endl;
            break;
        } else {
            best_move = best_move_this_search;
            max_score = alpha;
            std::cout << " bestmove " << moveToString(best_move) << " score " << max_score << std::endl;
        }
    }

    return best_move;
}

int Search::negaMax(Position &pos, int depth, int ply, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    } else if (pos.isTripleRepetition()) {
        return 0;
    } else if (depth == 0) {
        return qSearch(pos, depth, ply, alpha, beta);
    }

    nodes_searched++;

    int score = 0;
    move16 best_move = 0;

    if (tt.getScore(pos.z_key, depth, ply, alpha, beta, score, best_move)) {
        tt_hits++;
        return score;
    }

    MoveList moves;
    
    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    // check for stalemate or checkmate
    if (moves.size() == 0) {
        if (pos.in_check) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    orderMoves(pos, moves, best_move);
    
    int max_score = NEGATIVE_INFINITY;

    for (const auto &move: moves) {
        pos.makeMove(move);
        score = -negaMax(pos, depth - 1, ply + 1, -beta, -alpha);
        if (score > max_score) {
            best_move = move;
            max_score = score;
        }
        // max_score = std::max(max_score, -negaMax(pos, depth - 1, ply + 1, -beta, -alpha));
        alpha = std::max(alpha, max_score);
        pos.unmakeMove();
        if (max_score >= beta) {
            if (timesUp()) {
                return 0;
            } 
            tt.save(pos.z_key, depth, ply, move, max_score, LOWER_BOUND_NODE, pos.half_moves);
            return beta;
        }
    }

    if (timesUp()) {
        return 0;
    }
    tt.save(pos.z_key, depth, ply, best_move, max_score, EXACT_NODE, pos.game_half_moves);
    return max_score;
}

int Search::qSearch(Position &pos, int depth, int ply, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    }

    nodes_searched++;

    int score = 0;
    move16 best_move = 0;

    // if (tt.getScore(pos.z_key, depth, ply, alpha, beta, score, best_move)) {
    //     tt_hits++;
    //     return score;
    // }

    MoveList moves;
    
    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    // check for stalemate or checkmate
    if (moves.size() == 0) {
        if (pos.in_check) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }
    
    int max_score = eval(pos);

    if (max_score >= beta) {
        return beta;
    } else if (max_score > alpha) {
        alpha = max_score;
    }

    orderMoves(pos, moves, 0);

    for (const auto &move: moves) {
        if (getMoveType(move) != capture_move) {
            continue;
        }
        pos.makeMove(move);
        score = -qSearch(pos, depth - 1, ply + 1, -beta, -alpha);
        pos.unmakeMove();
        if (score >= beta) {
            // tt.save(pos.z_key, depth, ply, move, score, LOWER_BOUND_NODE, pos.half_moves);
            return beta;
        }
        if (score > max_score) {
            best_move = move;
            max_score = score;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    // tt.save(pos.z_key, depth, ply, best_move, max_score, EXACT_NODE, pos.game_half_moves);
    return max_score;
};