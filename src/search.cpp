#include "search.hpp"

#include <algorithm>

// copy the pv from ply + 1 to ply and add the first move to the front
void PVTable::updatePV(int ply, move16 first_move) {
    std::copy(table[ply + 1].begin(), table[ply + 1].begin() + pv_length[ply + 1], table[ply].begin() + 1);
    pv_length[ply] = pv_length[ply + 1] + 1;
    table[ply][0] = first_move;
}

// used in TT hits to update the pv
void PVTable::updateFromTT(int ply, move16 first_move) {
    pv_length[ply] = 1;
    table[ply][0] = first_move;
}

void PVTable::clearPV() {
    for (auto &a: table) {
        std::fill(a.begin(), a.end(), 0);
    }
    std::fill(pv_length.begin(), pv_length.end(), 0);
}

// sets the length of a particular pv index to zero (used before searching each ply)
void PVTable::zeroLength(int ply) {
    pv_length[ply] = 0;
}

Search::Search(): start_time(std::chrono::steady_clock::now()), tt_hits(0), allow_nmp(true), enable_qsearch_tt(true) {
    for (int i = 0; i < MAX_DEPTH; i++) {
        killer_1[i] = 0;
        killer_2[i] = 0;
    }
}

void Search::clearTT() {
    tt.clear();
}

void Search::setTimer(U64 duration_in_ms, int interval) {
    time_check_interval = interval;
    timer_duration = duration_in_ms;
    time_check = interval;
    times_up = false;
    start_time = std::chrono::steady_clock::now();
}

// Checks if the search time has expired
bool Search::timesUp() {
    if (times_up) {
        return true;
    } else if (node_search) {
        if (nodes_searched >= max_nodes) {
            times_up = true;
            return true;
        }
        return false;
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

// Output the search info in the UCI format
void Search::outputInfo(int depth, move16 best_move, int score, int nps) {
    std::cout << "info depth " << depth + 1 << " nodes " << nodes_searched;
    std::cout << " nps "<< nps << " bestmove " << moveToString(best_move) << " pv ";
    for (const auto &m: pv) {
        std::cout << moveToString(m) << " ";
    }
    std::cout << " score " << score << std::endl;
}

// Iterative deepening framework
SearchResult Search::iterSearch(Position &pos, int max_depth, U64 time_in_ms) {
    node_search = false;
    enable_qsearch_tt = true;
    setTimer(time_in_ms, 3000);
    tt_hits = 0;
    MoveList moves;
    SearchResult result;
    // pos.clearHistoryTable();
    //pv.clearPV();

    // Generate moves. This is only done once for the root node.
    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    move16 best_move = 0;
    int max_score = NEGATIVE_INFINITY;

    // Perform search at increasing depths
    bool research = false;
    for (int depth = 0; depth < max_depth; depth++) {
        std::chrono::steady_clock::time_point this_depth_start_time = std::chrono::steady_clock::now();
        if (depth == 0) {
            pv_search = false;
        } else {
            pv_search = true;
        }
        nodes_searched = 1;
        int beta;
        int alpha;
        int best_score_this_search_depth = NEGATIVE_INFINITY;
        move16 best_move_this_search_depth = 0;
        allow_nmp = true;

        // if this is not a re-search we order moves and set alpha and beta to their min and max values
        if (!research) {
            orderMoves(pos, moves, best_move, 0, 0);
            if (depth > 3) {
                // set aspiration window
                beta = max_score + WINDOW_SIZE;
                alpha = max_score - WINDOW_SIZE;
            } else {
                beta = POSITIVE_INFINITY;
                alpha = NEGATIVE_INFINITY;
            }
        }

        // Search from the root node using our pre-generated move list
        SearchResult result_this_depth = rootSearch(pos, moves, depth, alpha, beta);
        best_score_this_search_depth = result_this_depth.score;
        best_move_this_search_depth = result_this_depth.move;

        std::chrono::duration<double> time_elapsed = std::chrono::steady_clock::now() - this_depth_start_time;
        U64 nps = nodes_searched / time_elapsed.count();

        if (timesUp()) {
            if (best_score_this_search_depth > max_score) {
                max_score = best_score_this_search_depth;
                best_move = best_move_this_search_depth;
            }
            outputInfo(depth, best_move, max_score, nps);
            break;
        }

        // check to see if our score fell outside the aspiration window. Re-search if needed.
        if (best_score_this_search_depth <= alpha) {
            alpha = NEGATIVE_INFINITY;
            research = true;
            depth--;
            continue;
        } else if (best_score_this_search_depth >= beta) {
            beta = POSITIVE_INFINITY;
            research = true;
            depth--;
            continue;
        } else {
            research = false;
        }
        best_move = best_move_this_search_depth;
        max_score = best_score_this_search_depth;
        outputInfo(depth, best_move, max_score, nps);
    }

    result.move = best_move;
    result.score = max_score;

    return result;
}

// Function that gets called to search from the root node
SearchResult Search::rootSearch(Position &pos, MoveList &moves, int depth, int alpha, int beta) {
    SearchResult result;
    result.score = NEGATIVE_INFINITY;
    result.move = 0;
    for (const auto &move: moves) {
        pv.zeroLength(1);
        pos.makeMove(move);
        int score = -negaMax(pos, depth, 1, -beta, -alpha);
        pos.unmakeMove();
        if (timesUp()) {
            return result;
        }
        if (score > result.score) {
            result.score = score;
            result.move = move;
            if (score > alpha && score < beta) {
                pv.updatePV(0, move);
            }
            alpha = std::max(score, alpha);
        }
    }

    return result;
}

int Search::negaMax(Position &pos, int depth, int ply, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    } else if (pos.isTripleRepetition()) {
        return 0;
    } else if (depth == 0) {
        if (pos.side_to_move == WHITE) {
            if (inCheck<true>(pos))  {
                depth++;
            } else {
                return qSearch(pos, depth, ply, alpha, beta);
            } 
        } else {
            if (inCheck<false>(pos))  {
                depth++;
            } else {
                return qSearch(pos, depth, ply, alpha, beta);
            } 
        }
    }


    nodes_searched++;

    int score = 0;
    move16 best_move = 0;

    // Probe transposition table
    if (tt.getScore(pos.z_key, depth, ply, alpha, beta, score, best_move)) {
        tt_hits++;
        pv.updateFromTT(ply, best_move);
        return score;
    }

    // If we are searching the previous PV, put the PV move first
    if (pv_search) {
        if (ply < pv.length()) {
            best_move = pv.getPVMove(ply);
        } else {
            pv_search = false;
        }
    }

    MoveList moves;
    
    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    // null move pruning
    if (depth >= NMP_REDUCTION && allow_nmp && !pos.in_check && !pv_search) {
        allow_nmp = false;
        pos.makeNullMove();
        int nmp = -negaMax(pos, depth - NMP_REDUCTION, ply + 1, -beta, -beta + 1);
        pos.unmakeNullMove();
        allow_nmp = true;

        if (timesUp()) {
            return 0;
        }
        if (nmp >= beta) {
            return beta;
        }
    }

    // check for stalemate or checkmate
    if (moves.size() == 0) {
        if (pos.in_check) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    // MovePicker move_picker(pos, moves, best_move, killer_1[ply], killer_2[ply]);

    orderMoves(pos, moves, best_move, killer_1[ply], killer_2[ply]);
    
    int max_score = NEGATIVE_INFINITY;

    bool upper_bound = true;
    for (const auto &move: moves) {
        // set the following PV length to 0 in case the next node is a leaf node
        pv.zeroLength(ply + 1);
        pos.makeMove(move);
        score = -negaMax(pos, depth - 1, ply + 1, -beta, -alpha);
        pos.unmakeMove();
        if (score > max_score) {
            if (timesUp()) {
                return 0;
            } 
            best_move = move;
            pv.updatePV(ply, move);
            max_score = score;
            if (max_score >= beta) {
                if (!((move >> 12) & CAPTURE_MOVE)) {
                    saveKiller(ply, move);
                }
                tt.save(pos.z_key, depth, ply, move, max_score, LOWER_BOUND_NODE, pos.game_half_moves);
                return beta;
            }
            if (score > alpha) {
                alpha = score;
                upper_bound = false;
            }
        }
    }

    if (timesUp()) {
        return 0;
    }

    if (upper_bound) {
        tt.save(pos.z_key, depth, ply, best_move, max_score, UPPER_BOUND_NODE, pos.game_half_moves);
    } else {
        tt.save(pos.z_key, depth, ply, best_move, max_score, EXACT_NODE, pos.game_half_moves);
    }
    
    return max_score;
}

// quiescence search
int Search::qSearch(Position &pos, int depth, int ply, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    }

    nodes_searched++;

    int score = 0;
    move16 best_move = 0;

    if (enable_qsearch_tt && tt.getScore(pos.z_key, depth, ply, alpha, beta, score, best_move)) {
        tt_hits++;
        return score;
    }

    if (pv_search) {
        if (ply < pv.length()) {
            best_move = pv.getPVMove(ply);
        } else {
            pv_search = false;
        }
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
    
    int max_score = eval(pos);
    bool upper_bound = true;

    if (max_score >= beta) {
        return beta;
    } else if (max_score > alpha) {
        alpha = max_score;
        upper_bound = false;
    }

    orderMoves(pos, moves, best_move, 0, 0);
    // MovePicker move_picker(pos, moves, best_move, killer_1[ply], killer_2[ply]);
    best_move = 0;

    for (const auto &move: moves) {
        if (getMoveType(move) != CAPTURE_MOVE) {
            continue;
        }
        pv.zeroLength(ply + 1);
        pos.makeMove(move);
        score = -qSearch(pos, depth - 1, ply + 1, -beta, -alpha);
        pos.unmakeMove();
        if (timesUp()) return 0;
        if (score >= beta) {
            tt.save(pos.z_key, depth, ply, move, score, LOWER_BOUND_NODE, pos.game_half_moves);
            return beta;
        }
        if (score > max_score) {
            best_move = move;
            pv.updatePV(ply, move);
            max_score = score;
        }
        if (score > alpha) {
            alpha = score;
            upper_bound = false;
        }
    }

    if (upper_bound) {
        tt.save(pos.z_key, depth, ply, best_move, max_score, UPPER_BOUND_NODE, pos.game_half_moves);
    } else {
        tt.save(pos.z_key, depth, ply, best_move, max_score, EXACT_NODE, pos.game_half_moves);
    }
    return max_score;
};

int Search::qScore(Position &pos) {
    setTimer(1000, 1000);
    pv_search = false;
    node_search = false;
    enable_qsearch_tt = false;
    
    return qSearch(pos, 0, 0, NEGATIVE_INFINITY, POSITIVE_INFINITY);
}


//search a fixed number of nodes
SearchResult Search::nodeSearch(Position &pos, int max_depth, U64 num_nodes) {
    node_search = true;
    enable_qsearch_tt = true;
    times_up = false;
    max_nodes = num_nodes;
    tt_hits = 0;
    MoveList moves;
    SearchResult result;
    // pos.clearHistoryTable();
    //pv.clearPV();

    // Generate moves. This is only done once for the root node.
    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    move16 best_move = 0;
    int max_score = NEGATIVE_INFINITY;

    // Perform search at increasing depths
    bool research = false;
    for (int depth = 0; depth < max_depth; depth++) {
        std::chrono::steady_clock::time_point this_depth_start_time = std::chrono::steady_clock::now();
        if (depth == 0) {
            pv_search = false;
        } else {
            pv_search = true;
        }
        nodes_searched = 1;
        int beta;
        int alpha;
        int best_score_this_search_depth = NEGATIVE_INFINITY;
        move16 best_move_this_search_depth = 0;
        allow_nmp = true;

        // if this is not a re-search we order moves and set alpha and beta to their min and max values
        if (!research) {
            orderMoves(pos, moves, best_move, 0, 0);
            if (depth > 3) {
                // set aspiration window
                beta = max_score + WINDOW_SIZE;
                alpha = max_score - WINDOW_SIZE;
            } else {
                beta = POSITIVE_INFINITY;
                alpha = NEGATIVE_INFINITY;
            }
        }

        // Search from the root node using our pre-generated move list
        SearchResult result_this_depth = rootSearch(pos, moves, depth, alpha, beta);
        best_score_this_search_depth = result_this_depth.score;
        best_move_this_search_depth = result_this_depth.move;

        std::chrono::duration<double> time_elapsed = std::chrono::steady_clock::now() - this_depth_start_time;
        U64 nps = nodes_searched / time_elapsed.count();

        if (timesUp()) {
            if (best_score_this_search_depth > max_score) {
                max_score = best_score_this_search_depth;
                best_move = best_move_this_search_depth;
            }
            outputInfo(depth, best_move, max_score, nps);
            break;
        }

        // check to see if our score fell outside the aspiration window. Re-search if needed.
        if (best_score_this_search_depth <= alpha) {
            alpha = NEGATIVE_INFINITY;
            research = true;
            depth--;
            continue;
        } else if (best_score_this_search_depth >= beta) {
            beta = POSITIVE_INFINITY;
            research = true;
            depth--;
            continue;
        } else {
            research = false;
        }
        best_move = best_move_this_search_depth;
        max_score = best_score_this_search_depth;
        outputInfo(depth, best_move, max_score, nps);
    }

    result.move = best_move;
    result.score = max_score;

    return result;
};