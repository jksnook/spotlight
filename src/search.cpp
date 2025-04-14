#include "search.hpp"
#include "movepicker.hpp"

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

Search::Search(): start_time(std::chrono::steady_clock::now()), tt_hits(0), allow_nmp(true), enable_qsearch_tt(true), q_nodes(0), make_output(true) {
    for (int i = 0; i < MAX_PLY; i++) {
        killer_1[i] = 0;
        killer_2[i] = 0;
    }
}

void Search::clearTT() {
    tt.clear();
}

void Search::clearKillers() {
    for (int i = 0; i < MAX_PLY; i++) {
        killer_1[i] = 0;
        killer_2[i] = 0;
    }
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
    std::cout << "info depth " << depth << " nodes " << nodes_searched;
    std::cout << " nps "<< nps << " bestmove " << moveToString(best_move) << " pv ";
    for (const auto &m: pv) {
        std::cout << moveToString(m) << " ";
    }
    std::cout << " score " << score << std::endl;
}

// Timed search
SearchResult Search::timeSearch(Position &pos, int max_depth, U64 time_in_ms) {
    node_search = false;
    setTimer(time_in_ms, 3000);
    return iterSearch(pos, max_depth);
}

//search a fixed number of nodes
SearchResult Search::nodeSearch(Position &pos, int max_depth, U64 num_nodes) {
    node_search = true;
    times_up = false;
    max_nodes = num_nodes;
    return iterSearch(pos, max_depth);
};

// Iterative deepening framework
SearchResult Search::iterSearch(Position &pos, int max_depth) {
    total_nodes = 0ULL;
    q_nodes = 0ULL;
    enable_qsearch_tt = true;
    tt_hits = 0;

    SearchResult result;

    pv.clearPV();
    clearKillers();

    move16 best_move = 0;
    int best_score = NEGATIVE_INFINITY;

    // Perform search at increasing depths
    bool research = false;
    for (int depth = 1; depth <= max_depth; depth++) {
        // record start time
        std::chrono::steady_clock::time_point this_depth_start_time = std::chrono::steady_clock::now();

        if (depth == 1) {
            pv_search = false;
        } else {
            pv_search = true;
            // copy the pv so we can use it in the search
            if (!research) {
                old_pv = pv.table[0];
                old_pv_length = pv.pv_length[0];
            }
        }

        nodes_searched = 0;
        int beta;
        int alpha;
        int best_score_this_search_depth = NEGATIVE_INFINITY;
        move16 best_move_this_search_depth = 0;
        allow_nmp = true;

        // if this is not a re-search we set alpha and beta to their min and max values
        if (!research) {
            if (depth > WINDOW_MIN_DEPTH) {
                // set aspiration window
                beta = best_score + WINDOW_SIZE;
                alpha = best_score - WINDOW_SIZE;
            } else {
                beta = POSITIVE_INFINITY;
                alpha = NEGATIVE_INFINITY;
            }
        }

        // Search from the root node
        best_score_this_search_depth = negaMax(pos, depth, 0, alpha, beta);
        best_move_this_search_depth = pv.getPVMove(0);

        // record the time at the end of the search
        std::chrono::duration<double> time_elapsed = std::chrono::steady_clock::now() - this_depth_start_time;
        U64 nps = nodes_searched / time_elapsed.count();
        total_nodes += nodes_searched;

        if (timesUp()) {
            if (best_move_this_search_depth != best_move) {
                best_move = best_move_this_search_depth;
                if (make_output) outputInfo(depth, best_move, best_score, nps);
            }
            break;
        }

        // check to see if our score fell outside the aspiration window. Re-search if needed.
        if (best_score_this_search_depth <= alpha) {
            assert(alpha != NEGATIVE_INFINITY);
            alpha = NEGATIVE_INFINITY;
            research = true;
            depth--;
            continue;
        } else if (best_score_this_search_depth >= beta) {
            assert(beta != POSITIVE_INFINITY);
            beta = POSITIVE_INFINITY;
            research = true;
            depth--;
            continue;
        } else {
            research = false;
        }

        best_move = best_move_this_search_depth;
        best_score = best_score_this_search_depth;
        if (make_output) outputInfo(depth, best_move, best_score, nps);
    }

    result.move = best_move;
    result.score = best_score;

    return result;
}

int Search::negaMax(Position &pos, int depth, int ply, int alpha, int beta) {
    const bool is_root = ply == 0;

    if (timesUp()) {
        return 0;
    } else if (pos.isTripleRepetition()) {
        return 0;
    }

    bool in_check = inCheck(pos);

    if (in_check) depth++;
    
    if (depth == 0) {
        return qSearch(pos, depth, ply, alpha, beta);
    }

    assert(depth > 0);

    nodes_searched++;

    int score = 0;
    move16 tt_move = 0;

    const bool pv_node = beta - alpha > 1;;

    // If we are searching the previous PV, put the PV move first
    if (pv_search) {
        if (ply < old_pv_length) {
            tt_move = old_pv[ply];
        } else {
            pv_search = false;
        }
    }

    // Probe transposition table
    if (!pv_search && tt.getScore(pos.z_key, depth, ply, alpha, beta, score, tt_move)) {
        tt_hits++;
        pv.updateFromTT(ply, tt_move);
        return score;
    }

    int s_eval = eval(pos);

    eval_stack[ply] = s_eval;

    bool improving = ply < 2 || eval_stack[ply - 2] <= s_eval;

    //reverse futility pruning
    if (depth <= 4 && !is_root && !pv_node && !in_check) {
        int margin = 135 * depth;
        if (s_eval - margin >= beta) {
            return beta;
        }
    }

    // null move pruning
    if (depth >= 3 && !pv_node && allow_nmp && !pv_search && beta < POSITIVE_INFINITY && s_eval > beta &&
        !is_root && !in_check && pos.zugzwangUnlikely()) {
        assert(beta != POSITIVE_INFINITY);
        allow_nmp = false;
        int reduction = 3 + depth / 4;
        reduction = std::min(reduction, depth - 1);
        pos.makeNullMove();
        int nmp = -negaMax(pos, depth - reduction, ply + 1, -beta, -beta + 1);
        pos.unmakeNullMove();
        allow_nmp = true;

        if (timesUp()) {
            return 0;
        }
        if (nmp >= beta) {
            return beta;
        }
    }

    MovePicker move_picker(pos, tt_move, killer_1[ply], killer_2[ply]);
    
    int best_score = NEGATIVE_INFINITY;
    move16 best_move = 0;

    MoveList bad_quiets;

    bool can_fprune = false;

    // enable or disable futility pruning
    if (!is_root && !pv_node && depth == 1 && !in_check && alpha < MATE_THRESHOLD && 
        alpha > -MATE_THRESHOLD && beta < MATE_THRESHOLD && beta > -MATE_THRESHOLD && 
        s_eval + FUTILITY_MARGIN < alpha) {
        can_fprune = true;
    }

    // enable or disable late move reductions
    bool allow_lmr = !is_root && depth > 2 && !in_check;
    // enable or disable late move pruning
    bool allow_lmp = !is_root && depth <= 3 && !in_check && !pv_node;

    killer_1[ply + 1] = 0;
    killer_2[ply + 1] = 0;

    bool upper_bound = true;
    move16 move;
    int num_moves = 0;
    while (move = move_picker.getNextMove()) {
        num_moves++;
        if (num_moves > 1) pv_search = false;
        // futility pruning
        if (can_fprune && num_moves > 1 && !(getMoveType(move) & CAPTURE_MOVE || getMoveType(move) & PROMOTION_FLAG)) continue;

        // set the following PV length to 0 in case the next node is a leaf node
        pv.zeroLength(ply + 1);
        pos.makeMove(move);

        // Late move pruning
        if (allow_lmp && num_moves > 3 * depth + 2 * improving && !inCheck(pos)) {
            pos.unmakeMove();
            continue;
        }

        if (allow_lmr && (num_moves > 3 || (!improving && num_moves > 2))) {
            assert(depth > 0);
            int reduction = 2;
            if (num_moves > 6 && depth >= 4) {
                reduction = 3;
            }
            score = -negaMax(pos, depth - reduction, ply + 1, -alpha - 1, -alpha);
            if (score > alpha) {
                score = -negaMax(pos, depth - 1, ply + 1, -beta, -alpha);
            }
        } else if (pv_node && num_moves > 1) {
            score = -negaMax(pos, depth - 1, ply + 1, -alpha - 1, -alpha);
            if (score > alpha) {
                score = -negaMax(pos, depth - 1, ply + 1, -beta, -alpha);
            }
        } else {
            score = -negaMax(pos, depth - 1, ply + 1, -beta, -alpha);
        }
        pos.unmakeMove();
        if (score > best_score) {
            if (timesUp()) {
                return 0;
            } 
            best_move = move;
            pv.updatePV(ply, move);
            best_score = score;
            if (best_score >= beta) {
                if (!(getMoveType(move) & CAPTURE_MOVE)) {
                    saveKiller(ply, move);
                    pos.updateHistory(getFromSquare(move), getToSquare(move), depth * depth);
                }
                for (const auto &bq: bad_quiets) {
                    pos.updateHistory(getFromSquare(bq), getToSquare(bq), -depth * depth);
                }
                tt.save(pos.z_key, depth, ply, move, best_score, LOWER_BOUND_NODE, pos.game_half_moves);
                return beta;
            }
            if (score > alpha) {
                alpha = score;
                upper_bound = false;
            }
        }
        if(!(getMoveType(move) & CAPTURE_MOVE)) {
            bad_quiets.addMove(move);
        }
    }

    if (num_moves == 0) {
        if (in_check) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    if (timesUp()) {
        return 0;
    }

    if (upper_bound) {
        // reuse tt move in fail lows
        if (tt_move) best_move = tt_move;
        tt.save(pos.z_key, depth, ply, best_move, best_score, UPPER_BOUND_NODE, pos.game_half_moves);
    } else {
        tt.save(pos.z_key, depth, ply, best_move, best_score, EXACT_NODE, pos.game_half_moves);
    }

    assert(best_score != NEGATIVE_INFINITY);
    assert(best_score != POSITIVE_INFINITY);
    
    return best_score;
}

// quiescence search
int Search::qSearch(Position &pos, int depth, int ply, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    }

    nodes_searched++;
    q_nodes++;

    pv.zeroLength(ply);

    int score = 0;
    move16 tt_move = 0;

    const bool pv_node = beta - alpha > 1;

    if (pv_search) {
        if (ply < old_pv_length) {
            tt_move = old_pv[ply];
        } else {
            pv_search = false;
        }
    }

    if (enable_qsearch_tt && !pv_search && tt.getScore(pos.z_key, depth, ply, alpha, beta, score, tt_move)) {
        tt_hits++;
        return score;
    }

    bool in_check = inCheck(pos);

    if (!in_check && !(getMoveType(tt_move) & CAPTURE_MOVE || getMoveType(tt_move) & PROMOTION_FLAG)) { 
        tt_move = 0;
        pv_search = false;
    }

    int s_eval = eval(pos);
    int best_score;
    
    if (!in_check) {
        best_score = s_eval;
    } else {
        best_score = NEGATIVE_INFINITY;
    }

    bool upper_bound = true;

    if (best_score >= beta) {
        pv_search = false;
        return beta;
    } else if (best_score > alpha) {
        alpha = best_score;
        upper_bound = false;
    }

    MovePicker move_picker(pos, tt_move, 0, 0);

    int num_moves = 0;

    move16 move;
    move16 best_move = 0;


    while (in_check ? move = move_picker.getNextMove() : move = move_picker.getNextCapture()) {
        num_moves++;
        // if (!in_check && !(getMoveType(move) & CAPTURE_MOVE || getMoveType(move) & PROMOTION_FLAG)) continue;
        if (num_moves > 1) pv_search = false;
        // delta pruning. Formula is kind of a kludge as SEE values and eval are not really comparable
        if (!isQuiet(move) && see(pos, move) <= std::max((alpha - s_eval) * SEE_MULTIPLIER - SEE_MULTIPLIER * 80, 0)) continue;
        pos.makeMove(move);
        score = -qSearch(pos, depth - 1, ply + 1, -beta, -alpha);
        pos.unmakeMove();
        if (timesUp()) return 0;
        if (score >= beta) {
            tt.save(pos.z_key, depth, ply, move, score, LOWER_BOUND_NODE, pos.game_half_moves);
            return beta;
        }
        if (score > best_score) {
            best_move = move;
            pv.updatePV(ply, move);
            best_score = score;
        }
        if (score > alpha) {
            alpha = score;
            upper_bound = false;
        }
    }

    pv_search = false;

    // in case we are in check and every move is a capture
    if (best_score == NEGATIVE_INFINITY) {
        best_score = s_eval;
    }

    if (num_moves == 0 && !move_picker.getNextMove()) {
        if (inCheck(pos)) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    if (upper_bound) {
        if (tt_move) best_move = tt_move;
        tt.save(pos.z_key, depth, ply, best_move, best_score, UPPER_BOUND_NODE, pos.game_half_moves);
    } else {
        tt.save(pos.z_key, depth, ply, best_move, best_score, EXACT_NODE, pos.game_half_moves);
    }
    return best_score;
};

int Search::qScore(Position &pos) {
    setTimer(1000, 1000);
    pv_search = false;
    node_search = false;
    enable_qsearch_tt = false;
    
    return qSearch(pos, 0, 0, NEGATIVE_INFINITY, POSITIVE_INFINITY);
}
