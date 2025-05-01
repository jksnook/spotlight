#include "search.hpp"
#include "movepicker.hpp"

#include <algorithm>

namespace Spotlight {

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

Search::Search(TT* _tt, std::atomic<bool>* _is_stopped): tt(_tt), is_stopped(_is_stopped),
start_time(std::chrono::steady_clock::now()), tt_hits(0), allow_nmp(true), enable_qsearch_tt(true), 
q_nodes(0), make_output(true), times_up(false), thread_id(0) {
    clearHistory();
    for (int i = 0; i < MAX_PLY; i++) {
        for (int k = 0; k < 256; k++) {
            lmr_table[i][k] = log(i) * log(k) / 2.5 + 1.8;
        } 

        killer_1[i] = 0;
        killer_2[i] = 0;
    }
}

void Search::clearTT() {
    tt->clear();
}


void Search::clearKillers() {
    for (int i = 0; i < MAX_PLY; i++) {
        killer_1[i] = 0;
        killer_2[i] = 0;
    }
}

void Search::clearHistory() {
    for (auto &side: quiet_history) {
        for (auto &start: side) {
            for (auto &end: start) {
                end = 0;
            }
        }
    }
}

void Search::updateHistory(Color side, int from, int to, int bonus) {
    quiet_history[side][from][to] += bonus - abs(bonus) * quiet_history[side][from][to] / MAX_HISTORY;
}

void Search::setTimer(U64 duration_in_ms, int interval) {
    time_check_interval = interval;
    timer_duration = duration_in_ms;
    soft_time_limit = timer_duration * 3 / 4;
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
    if (is_stopped->load()) {
        times_up = true;
        return true;
    }
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    if (time_elapsed.count() > timer_duration) {
        is_stopped->store(true);
        times_up = true;
        return true;
    }
    time_check = time_check_interval;
    return false;
}

bool Search::softTimesUp() {
    if (node_search) {
        return false;
    }
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time);
    if (time_elapsed.count() > soft_time_limit) {
        times_up = true;
        is_stopped->store(true);
        return true;
    }
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
    setTimer(time_in_ms, 1000);
    return iterSearch(pos, max_depth);
}

//search a fixed number of nodes
SearchResult Search::nodeSearch(Position &pos, int max_depth, U64 num_nodes) {
    node_search = true;
    times_up = false;
    max_nodes = num_nodes;
    return iterSearch(pos, max_depth);
};

SearchResult Search::iterSearch(Position& pos, int max_depth) {
    total_nodes = 0ULL;
    q_nodes = 0ULL;
    enable_qsearch_tt = true;
    tt_hits = 0;

    pv.clearPV();
    clearKillers();

    move16 best_move = 0;
    int best_score = NEGATIVE_INFINITY;
    int beta = 30;
    int alpha = -30;

    bool research = false;
    for (int depth = 1; depth <= max_depth; depth++) {
        // record start time
        std::chrono::steady_clock::time_point this_depth_start_time = std::chrono::steady_clock::now();

        if (depth == 1) {
            search_previous_pv = false;
        } else {
            search_previous_pv = true;
            // copy the pv so we can use it in the search
            if (!research) {
                old_pv = pv.table[0];
                old_pv_length = pv.pv_length[0];
            }
        }

        nodes_searched = 0;
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
        assert(depth <= MAX_PLY);
        best_score_this_search_depth = negaMax<true, false, true>(pos, depth, 0, alpha, beta);
        best_move_this_search_depth = pv.getPVMove(0);

        // record the time at the end of the search
        std::chrono::duration<double> time_elapsed = std::chrono::steady_clock::now() - this_depth_start_time;
        U64 nps = nodes_searched / time_elapsed.count();
        total_nodes += nodes_searched;

        if (timesUp()) {
            if (best_move_this_search_depth != best_move) {
                best_move = best_move_this_search_depth;
                if (make_output && thread_id == 0) outputInfo(depth, best_move, best_score, nps);
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
        if (make_output && thread_id == 0) outputInfo(depth, best_move, best_score, nps);
        if (thread_id == 0 && softTimesUp()) break;

    }

    SearchResult result;

    result.move = best_move;
    result.score = best_score;

    if (thread_id == 0 && make_output) {
        std::cout << "bestmove " << moveToString(best_move) << std::endl;
    }

    return result;
}

template <bool t_pv_node, bool cut_node, bool t_is_root>
int Search::negaMax(Position& pos, int depth, int ply, int alpha, int beta) {
    if (ply >= MAX_PLY) {
        return eval(pos);
    }
    assert(depth <= MAX_PLY);

    const bool is_root = ply == 0;
    pv.zeroLength(ply);

    if (timesUp()) {
        return 0;
    } else if (pos.isTripleRepetition() || (!is_root && pos.fifty_move >= 50)) {
        return 0;
    }

    bool in_check = inCheck(pos);

    if (in_check) depth++;
    
    if (depth <= 0) {
        return qSearch(pos, 0, ply, alpha, beta);
    }

    assert(depth > 0);

    nodes_searched++;

    int score = 0;
    move16 tt_move = 0;

    const bool pv_node = beta - alpha > 1;

    // If we are searching the previous PV, put the PV move first
    if (search_previous_pv) {
        if (ply < old_pv_length) {
            tt_move = old_pv[ply];
        } else {
            search_previous_pv = false;
        }
    }

    // Probe transposition table
    TTEntry* tt_entry = tt->probe(pos.z_key);

    if (!search_previous_pv && tt_entry->node_type != NULL_NODE && tt_entry->z_key == pos.z_key) {
        tt_move = tt_entry->best_move;
        score = tt_entry->score;

        if (score > MATE_THRESHOLD) {
            score -= ply;
        } else if (score < -MATE_THRESHOLD) {
            score += ply;
        }

        if (tt_entry->depth >= depth && (tt_entry->node_type == EXACT_NODE || (tt_entry->node_type == LOWER_BOUND_NODE && score >= beta)
            || (tt_entry->node_type == UPPER_BOUND_NODE && score <= alpha))) {
            tt_hits++;
            pv.updateFromTT(ply, tt_move);
            return score;
        }
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
    if (depth >= 3 && !pv_node && allow_nmp && !search_previous_pv && beta < POSITIVE_INFINITY && s_eval > beta &&
        !is_root && !in_check && pos.zugzwangUnlikely()) {
        assert(beta != POSITIVE_INFINITY);
        allow_nmp = false;
        int reduction = 3 + depth / 4;
        reduction = std::min(reduction, depth - 1);
        pos.makeNullMove();
        int nmp = -negaMax<t_pv_node, true, false>(pos, depth - reduction, ply + 1, -beta, -beta + 1);
        pos.unmakeNullMove();
        allow_nmp = true;

        if (times_up) {
            return 0;
        } 
        if (nmp >= beta) {
            return beta;
        }
    }

    MovePicker move_picker(pos, &quiet_history, tt_move, killer_1[ply], killer_2[ply]);
    
    int best_score = NEGATIVE_INFINITY;
    move16 best_move = 0;

    MoveList bad_quiets;

    bool can_fprune = false;

    // enable or disable futility pruning
    if (!is_root && !pv_node && depth <= 2 && !in_check && 
        alpha > -MATE_THRESHOLD && beta < MATE_THRESHOLD && 
        s_eval + FUTILITY_MARGIN + (depth - 1) * 70 + 25 * improving < alpha) {
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
        if (num_moves > 1) search_previous_pv = false;
        // futility pruning
        if (can_fprune && num_moves > 1 && !(getMoveType(move) & CAPTURE_MOVE || getMoveType(move) & PROMOTION_FLAG)) continue;
        pos.makeMove(move);

        // tt prefetching
        __builtin_prefetch(tt->probe(pos.z_key));

        // Late move pruning
        if (allow_lmp && num_moves > 3 * depth + 2 * improving + 1 && !inCheck(pos)) {
            pos.unmakeMove();
            continue;
        }

        // Late move reductions

        int reduction = 0;
        score = NEGATIVE_INFINITY;
        if (allow_lmr && (num_moves > 2 || (!improving && num_moves > 1))) {
            assert(depth > 0);
            int reduction = lmr_table[depth][num_moves] - !isQuiet(move);
            score = -negaMax<false, true, false>(pos, depth - reduction, ply + 1, -alpha - 1, -alpha);
            if (score > alpha) {
                score = -negaMax<t_pv_node, !cut_node, false>(pos, depth - 1, ply + 1, -beta, -alpha);
            }
        } else if (pv_node && num_moves > 1) {
            score = -negaMax<false, true, false>(pos, depth - 1, ply + 1, -alpha - 1, -alpha);
            if (score > alpha) {
                score = -negaMax<true, false, false>(pos, depth - 1, ply + 1, -beta, -alpha);
            }
        } else {
            score = -negaMax<t_pv_node, !cut_node, false>(pos, depth - 1, ply + 1, -beta, -alpha);
        }

        pos.unmakeMove();

        if (times_up) return 0;

        if (score > best_score) {
            best_move = move;
            pv.updatePV(ply, move);
            best_score = score;

            if (best_score >= beta) {
                if (isQuiet(move)) {
                    saveKiller(ply, move);
                    updateHistory(pos.side_to_move, getFromSquare(move), getToSquare(move), depth * depth);
                }
                /* applying history malus even when a non-quiet move fails. I think this is
                non-standard but this is what worked. */
                for (const auto &bq: bad_quiets) {
                    updateHistory(pos.side_to_move, getFromSquare(bq), getToSquare(bq), -depth * depth);
                }
                tt->save(pos.z_key, depth, ply, move, best_score, LOWER_BOUND_NODE, pos.game_half_moves);
                return score;
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

    if (upper_bound) {
        // reuse tt move in fail lows
        if (tt_move) best_move = tt_move;
        tt->save(pos.z_key, depth, ply, best_move, best_score, UPPER_BOUND_NODE, pos.game_half_moves);
    } else {
        tt->save(pos.z_key, depth, ply, best_move, best_score, EXACT_NODE, pos.game_half_moves);
    }

    assert(best_score != NEGATIVE_INFINITY);
    assert(best_score != POSITIVE_INFINITY);
    
    return best_score;
}

int Search::qSearch(Position & pos, int depth, int ply, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    }
    
    else if (ply >= MAX_PLY || pos.fifty_move >= 50) {
        return eval(pos);
    }

    nodes_searched++;
    q_nodes++;

    pv.zeroLength(ply);

    int score = 0;
    move16 tt_move = 0;

    const bool pv_node = beta - alpha > 1;

    if (search_previous_pv) {
        if (ply < old_pv_length) {
            tt_move = old_pv[ply];
        } else {
            search_previous_pv = false;
        }
    }

    // Probe transposition table
    TTEntry* tt_entry = tt->probe(pos.z_key);

    if (enable_qsearch_tt && !search_previous_pv && tt_entry->node_type != NULL_NODE && tt_entry->z_key == pos.z_key) {
        tt_move = tt_entry->best_move;
        score = tt_entry->score;

        if (score > MATE_THRESHOLD) {
            score -= ply;
        } else if (score < -MATE_THRESHOLD) {
            score += ply;
        }

        if (tt_entry->depth >= depth && (tt_entry->node_type == EXACT_NODE || (tt_entry->node_type == LOWER_BOUND_NODE && score >= beta)
            || (tt_entry->node_type == UPPER_BOUND_NODE && score <= alpha))) {
            tt_hits++;
            return score;
        }
    }

    bool in_check = inCheck(pos);

    if (!in_check && !(getMoveType(tt_move) & CAPTURE_MOVE || getMoveType(tt_move) & PROMOTION_FLAG)) { 
        tt_move = 0;
        search_previous_pv = false;
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
        search_previous_pv = false;
        return best_score;
    } else if (best_score > alpha) {
        alpha = best_score;
        upper_bound = false;
    }

    MovePicker move_picker(pos, &quiet_history, tt_move, 0, 0);

    int num_moves = 0;

    move16 move;
    move16 best_move = 0;

    while (in_check ? move = move_picker.getNextMove() : move = move_picker.getNextCapture()) {
        num_moves++;
        if (num_moves > 1) search_previous_pv = false;
        // delta pruning. Formula is kind of a kludge as SEE values and eval are not really comparable
        if (!isQuiet(move) && see(pos, move) <= std::max((alpha - s_eval) * SEE_MULTIPLIER - SEE_MULTIPLIER * 80, 0)) continue;
        pos.makeMove(move);
        // tt prefetching
        __builtin_prefetch(tt->probe(pos.z_key));
        score = -qSearch(pos, depth - 1, ply + 1, -beta, -alpha);
        pos.unmakeMove();
        if (times_up) return 0;
        if (score >= beta) {
            tt->save(pos.z_key, depth, ply, move, score, LOWER_BOUND_NODE, pos.game_half_moves);
            return score;
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

    search_previous_pv = false;

    // in case we are in check and every move is a capture
    if (best_score == NEGATIVE_INFINITY) {
        best_score = s_eval;
    }

    if (num_moves == 0 && !move_picker.getNextMove()) {
        if (in_check) {
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    if (upper_bound) {
        if (tt_move) best_move = tt_move;
        tt->save(pos.z_key, depth, ply, best_move, best_score, UPPER_BOUND_NODE, pos.game_half_moves);
    } else {
        tt->save(pos.z_key, depth, ply, best_move, best_score, EXACT_NODE, pos.game_half_moves);
    }
    return best_score;
}

int Search::qScore(Position &pos) {
    setTimer(1000, 1000);
    search_previous_pv = false;
    node_search = false;
    enable_qsearch_tt = false;
    
    return qSearch(pos, 0, 0, NEGATIVE_INFINITY, POSITIVE_INFINITY);
}

} // namespace Spotlight
