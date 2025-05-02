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

// Iterative deepening framework
SearchResult Search::iterSearch(Position& pos, int max_depth) {
    total_nodes = 0ULL;
    q_nodes = 0ULL;
    enable_qsearch_tt = true;
    tt_hits = 0;

    pv.clearPV();
    clearKillers();

    max_depth = std::min(max_depth, MAX_PLY - 1);

    move16 best_move = NULL_MOVE;
    int best_score = 0;

    int beta = POSITIVE_INFINITY;
    int alpha = NEGATIVE_INFINITY;

    // tells us if we are re-searching due to falling outside the aspiration window
    bool research = false;

    for (int depth = 1; depth <= max_depth; depth++) {
        nodes_searched = 0ULL;

        // set aspiration windows
        // aspiration windows aren't gaining right now. I will reenable them once I add some more features.
        // if (depth > WINDOW_MIN_DEPTH && !research) {
        //     alpha = best_score - WINDOW_SIZE;
        //     beta = best_score + WINDOW_SIZE;
        // }

        int score = negaMax<true, false, true>(pos, depth, 0, alpha, beta);

        total_nodes += nodes_searched;

        // check for search timeout
        if (times_up) {
            // if the best move has changed we use it even if the search timed out
            if (pv.getPVMove(0) != best_move) {
                best_move = pv.getPVMove(0);
                best_score = score;
                if (make_output) outputInfo(depth, best_move, best_score, 0);
            }
            break;
        }

        // re-search if our score is outside the aspiration window
        // if (score <= alpha) {
        //     alpha = NEGATIVE_INFINITY;
        //     research = true;
        //     depth--;
        //     continue;
        // } else if (score >= beta) {
        //     beta = POSITIVE_INFINITY;
        //     research = true;
        //     depth--;
        //     continue;
        // }

        best_move = pv.getPVMove(0);
        best_score = score;

        if (make_output) outputInfo(depth, best_move, best_score, 0);

        if (softTimesUp()) break;
    }

    SearchResult result;

    result.move = best_move;
    result.score = best_score;

    if (thread_id == 0) std::cout << "bestmove " << moveToString(best_move) << std::endl;

    return result;
}

template <bool pv_node, bool cut_node, bool is_root>
int Search::negaMax(Position& pos, int depth, int ply, int alpha, int beta) {
    // clear the pv at this ply
    pv.zeroLength(ply);

    // check for exit conditions
    if (timesUp() || (!is_root && pos.isTripleRepetition())) {
        return 0;
    }

    bool in_check = inCheck(pos);

    // If we are at depth 0 then drop into the quiescence search
    if (depth <= 0) {
        return qSearch(pos, 0, ply, alpha, beta);
    }

    // Increase node count only after checking for exit conditions
    nodes_searched++;

    // Probe the transposition table
    move16 tt_move = NULL_MOVE;
    TTEntry* entry = tt->probe(pos.z_key);

    if (entry->node_type != NULL_NODE && entry->z_key == pos.z_key) {
        // grab the tt move for move ordering if the hash key matches
        tt_move = entry->best_move;

        // return the score from the TT if the bounds and depth allow it
        if (!pv_node && !is_root && entry->depth >= depth && (
            entry->node_type == EXACT_NODE ||
            (entry->node_type == LOWER_BOUND_NODE && entry->score >= beta) ||
            (entry->node_type == UPPER_BOUND_NODE && entry->score <= alpha)
            )) {
            return entry->score;
        }
    }

    // get static evaluation for use in pruning heuristics
    int s_eval = eval(pos);

    /*
    Reverse Futility Pruning
    If our eval is above beta + some margin we consider this a beta cutoff
    */
    if (!pv_node && !in_check && depth <= 3 && s_eval >= beta + 120 * depth) {
        return s_eval;
    }

    /*
    Null Move Pruning
    This takes advantage of the observation that playing a move is nearly
    always better than doing nothing. We disable it in positions where 
    zugzwang is likely
    */
    if (!pv_node && allow_nmp && depth > 3 && !in_check && pos.zugzwangUnlikely()) {
        // calculate depth reduction
        int reduction = 3 + depth / 4;
        // don't drop directly into quiescence search
        reduction = std::min(reduction, depth - 1);
        // apply a null move
        pos.makeNullMove();
        // disable null move pruning in our reduced search
        allow_nmp = false;
        // search with a reduced depth and null window
        int nmp = -negaMax<false, true, false>(pos, depth - reduction, ply + 1, -beta, -beta + 1);
        pos.unmakeNullMove();
        allow_nmp = true;

        // check for timeout
        if (times_up) return 0;

        // if our score is still above beta even after a null move we consider this a beta cutoff
        if (nmp >= beta) return beta;

    }

    // initialize move picker with appropriate data for move ordering
    MovePicker move_picker(pos, &quiet_history, tt_move, 0, 0);

    move16 move = NULL_MOVE;
    int best_score = NEGATIVE_INFINITY;
    move16 best_move = NULL_MOVE;
    int num_moves = 0;
    bool is_upper_bound = true;

    // List of all the quiet moves that didn't cause a beta cutoff. used for updating history
    MoveList bad_quiets;

    // Loop through all legal moves in the position and recursively call the search function
    while (move = move_picker.getNextMove()) {
        num_moves++;

        int score = 0;

        pos.makeMove(move);

        // TT prefetching. avoids cache misses that cause slow TT lookups
        __builtin_prefetch(tt->probe(pos.z_key));

        // Principal variation search
        // template parameters are <pv_node, cut_node, is_root>
        if (pv_node && num_moves == 1) {
            // always search the first move of a pv node with a full window
            score = -negaMax<pv_node, false, false>(pos, depth - 1, ply + 1, -beta, -alpha);
        } else {
            // search with a zero window
            score = -negaMax<false, !cut_node, false>(pos, depth - 1, ply + 1, -alpha - 1, -alpha);
            if (pv_node && score > alpha) {
                // re-search if we raise alpha in a pv node
                score = -negaMax<pv_node, false, false>(pos, depth - 1, ply + 1, -beta, -alpha);
            }
        }

        pos.unmakeMove();

        // check for timeout to avoid storing bad values in the TT
        if (times_up) return 0;

        if (score > best_score) {
            best_score = score;
            best_move = move;
            pv.updatePV(ply, move);
            // check for a beta cutoff
            if (score >= beta) {
                if (isQuiet(move)) {
                    // update butterfly history
                    int bonus = depth * depth;
                    updateHistory(pos.side_to_move, getFromSquare(move), getToSquare(move), bonus);
                    // apply malus to the previous quiet moves
                    for (auto& bq : bad_quiets) {
                        updateHistory(pos.side_to_move, getFromSquare(bq), getToSquare(bq), -bonus);
                    }
                }
                // Store to TT as a fail high
                tt->save(pos.z_key, depth, ply, move, score, LOWER_BOUND_NODE, pos.half_moves);
                // beta cutoff
                return score;
            }
            else if (score > alpha) {
                // raise alpha if necessary
                alpha = score;
                is_upper_bound = false;
            }
        }

        if (isQuiet(move)) bad_quiets.addMove(move);
    }

    // check for checkmate and stalemate
    if (num_moves == 0) {
        if (inCheck(pos)) {
            // subtract our current ply from the mate score to encourage faster checkmates
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    // save to TT as an upper bound node or an exact node depending on if we raised alpha
    if (is_upper_bound) {
        // re-use the old TT move in fail lows
        tt->save(pos.z_key, depth, ply, tt_move, best_score, UPPER_BOUND_NODE, pos.half_moves);
    } else {
        tt->save(pos.z_key, depth, ply, best_move, best_score, EXACT_NODE, pos.half_moves);
    }

    return best_score;
}

// quiescence search 
int Search::qSearch(Position& pos, int depth, int ply, int alpha, int beta) {
    if (timesUp()) {
        return 0;
    }

    pv.zeroLength(ply);
    nodes_searched++;
    q_nodes++;

    bool in_check = inCheck(pos);

    // tt disabled in qsearch for now
    move16 tt_move = NULL_MOVE;

    // get static eval;
    int stand_pat;
    
    // don't use standing pat when in check. (we need to search evasions)
    if (!in_check) {
        stand_pat = eval(pos);
    } else {
        stand_pat = NEGATIVE_INFINITY;
    }

    bool is_upper_bound = true;

    if (stand_pat >= beta) {
        // standing pat beta cutoff
        return stand_pat;
    }
    else if (stand_pat > alpha) {
        /*
        Use the static eval as a lower bound for our score.
        This is sound because in chess making a move is usually better than doing nothing
        */
        alpha = stand_pat;
        is_upper_bound = false;
    }

    MovePicker move_picker(pos, &quiet_history, tt_move, 0, 0);

    move16 move = NULL_MOVE;
    move16 best_move = NULL_MOVE;
    int best_score = stand_pat;
    int num_moves = 0;

    // if in check search all legal moves, otherwise search captures
    while (in_check ? move = move_picker.getNextMove() : move = move_picker.getNextCapture()) {
        num_moves++;
        pos.makeMove(move);
        int score = -qSearch(pos, depth - 1, ply + 1, -beta, -alpha);
        pos.unmakeMove();

        if (score > best_score) {
            best_score = score;
            best_move = move;
            if (score >= beta) {
                return score;
            } else if (score > alpha) {
                alpha = score;
                is_upper_bound = false;
            }
        }

    }

    // check for checkmate and stalemate
    if (num_moves == 0 && !move_picker.getNextMove()) {
        if (inCheck(pos)) {
            // subtract our current ply from the mate score to encourage faster checkmates
            return -MATE_SCORE + ply;
        }
        return 0;
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
