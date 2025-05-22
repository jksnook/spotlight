#include "search.hpp"

#include <algorithm>
#include <sstream>

#include "movepicker.hpp"

namespace Spotlight {

// copy the pv from ply + 1 to ply and add the first move to the front
void PVTable::updatePV(int ply, move16 first_move) {
    std::copy(table[ply + 1].begin(), table[ply + 1].begin() + pv_length[ply + 1],
              table[ply].begin() + 1);
    pv_length[ply] = pv_length[ply + 1] + 1;
    table[ply][0] = first_move;
}

// used in TT hits to update the pv
void PVTable::updateFromTT(int ply, move16 first_move) {
    pv_length[ply] = 1;
    table[ply][0] = first_move;
}

void PVTable::clearPV() {
    for (auto &a : table) {
        std::fill(a.begin(), a.end(), 0);
    }
    std::fill(pv_length.begin(), pv_length.end(), 0);
}

// sets the length of a particular pv index to zero (used before searching each ply)
void PVTable::zeroLength(int ply) { pv_length[ply] = 0; }

Search::Search(TT *_tt, std::atomic<bool> *_is_stopped, std::function<U64()> _getNodes)
    : tt_hits(0),
      nodes_searched(0),
      q_nodes(0),
      make_output(true),
      thread_id(0),
      is_stopped(_is_stopped),
      getNodes(_getNodes),
      node_search(false),
      allow_nmp(true),
      times_up(false),
      enable_qsearch_tt(true),
      start_time(std::chrono::steady_clock::now()),
      tt(_tt) {
    clearHistory();
    for (int i = 0; i < MAX_PLY; i++) {
        for (int k = 0; k < 256; k++) {
            /*
            Late move reductions calculated here

            indices are [depth][num_moves]
            */
            lmr_table[i][k] = log(i) * log(k) / 2.5 + 2.5;
        }

        // clear the killer moves
        killer_1[i] = 0;
        killer_2[i] = 0;
    }
}

void Search::clearTT() { tt->clear(); }

void Search::clearKillers() {
    for (int i = 0; i < MAX_PLY; i++) {
        killer_1[i] = 0;
        killer_2[i] = 0;
    }
}

void Search::clearHistory() {
    for (auto &side : quiet_history) {
        for (auto &start : side) {
            for (auto &end : start) {
                end = 0;
            }
        }
    }
}

// Update butterfly history using the history gravity formula
void Search::updateHistory(Color side, int from, int to, int bonus) {
    quiet_history[side][from][to] +=
        bonus - abs(bonus) * quiet_history[side][from][to] / MAX_HISTORY;
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
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    if (time_elapsed.count() > static_cast<int64_t>(timer_duration)) {
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
    auto time_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time);
    if (time_elapsed.count() > static_cast<int64_t>(soft_time_limit)) {
        times_up = true;
        is_stopped->store(true);
        return true;
    }
    return false;
}

// Output the search info in the UCI format
void Search::outputInfo(int depth, move16 best_move, int score) {
    std::stringstream ss;
    std::chrono::duration<double> time_elapsed = std::chrono::steady_clock::now() - start_time;
    U64 nodes = getNodes();
    U64 nps = nodes / time_elapsed.count();
    ss << "info depth " << depth;
    if (score > MATE_THRESHOLD || score < -MATE_THRESHOLD) {
        ss << " score mate " << (pv.length() + 1) / 2;
    } else {
        ss << " score cp " << score;
    }
    ss << " nodes " << nodes << " nps " << nps << " hashfull " << tt->hashfull();
    ss << " pv ";
    for (const auto &m : pv) {
        ss << moveToString(m) << " ";
    }
    std::cout << ss.str() << std::endl;
}

// Timed search
SearchResult Search::timeSearch(Position &pos, int max_depth, U64 time_in_ms) {
    node_search = false;
    setTimer(time_in_ms, 1000);
    return iterSearch(pos, max_depth);
}

// search a fixed number of nodes
SearchResult Search::nodeSearch(Position &pos, int max_depth, U64 num_nodes) {
    node_search = true;
    times_up = false;
    max_nodes = num_nodes;
    return iterSearch(pos, max_depth);
};

// Iterative deepening framework
SearchResult Search::iterSearch(Position &pos, int max_depth) {
    nodes_searched = 0ULL;
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

    for (int depth = 1; depth <= max_depth; depth++) {
        /*
        Aspiration Windows

        at first search with a narrow window to increase beta cutoffs.
        If we fail high or low we re-search with a wider window.
        */
        if (depth > WINDOW_MIN_DEPTH) {
            alpha = best_score - WINDOW_SIZE;
            beta = best_score + WINDOW_SIZE;
        }

        // delta used for widening aspiration windows
        int delta = WINDOW_SIZE;
        int score;

        while (true) {
            // call negaMax as a PV-node and root node at ply 0
            score = negaMax<true, false, true>(pos, depth, 0, alpha, beta);

            // check for search timeout
            if (times_up) {
                // if the best move has changed we use it even if the search timed out
                if (thread_id == 0 && pv.getPVMove(0) && pv.getPVMove(0) != best_move) {
                    best_move = pv.getPVMove(0);
                    best_score = score;
                    if (make_output) outputInfo(depth, best_move, best_score);
                }
                break;
            }

            // re-search if our score is outside the aspiration window
            if (score <= alpha) {
                beta = (alpha + beta) / 2;
                alpha -= delta;
            } else if (score >= beta) {
                alpha = (alpha + beta) / 2;
                beta += delta;
            } else {
                // break if score fell within the window
                break;
            }
            // increment delta for each re-search
            delta *= 2;
        }
        if (times_up) break;

        best_move = pv.getPVMove(0);
        best_score = score;

        if (make_output && thread_id == 0) outputInfo(depth, best_move, best_score);

        // If our soft time limit has expired we don't start another search iteration
        if (softTimesUp()) break;
    }

    SearchResult result;

    result.move = best_move;
    result.score = best_score;

    // assert(best_move);

    if (thread_id == 0 && make_output)
        std::cout << "bestmove " << moveToString(best_move) << std::endl;

    return result;
}

/*
The Main Search Function

Alpha-beta pruning in a negamax framework
*/
template <bool pv_node, bool cut_node, bool is_root>
int Search::negaMax(Position &pos, int depth, int ply, int alpha, int beta) {
    // check ply limit
    // we use MAX_PLY - 1 because some arrays are accessed with
    // index ply + 1
    if (ply >= MAX_PLY - 1) return eval(pos);

    // clear the pv at this ply
    pv.zeroLength(ply);

    // check for exit conditions
    if (timesUp() || (!is_root && (pos.isTripleRepetition() || pos.fifty_move >= 100))) {
        return 0;
    }

    bool in_check = inCheck(pos);

    // If we are at depth 0 then drop into the quiescence search
    if (depth <= 0) {
        return qSearch(pos, 0, ply, alpha, beta);
    }

    /*
    Mate Distance Pruning

    Don't bother looking past the current mate depth
    when a mate has been found
    */
    alpha = std::max(alpha, -MATE_SCORE + ply);
    beta = std::min(beta, MATE_SCORE - ply - 1);
    if (!is_root && alpha >= beta) return beta;

    // Increase node count only after checking for exit conditions
    nodes_searched++;

    move16 tt_move = NULL_MOVE;
    int s_eval;
    bool tt_hit = false;
    NodeType node_type;
    int tt_depth;
    int tt_score;
    bool tt_pv;

    // Probe the transposition table
    if ((tt_hit = tt->probe(pos.z_key, tt_move, node_type, tt_depth, tt_score, s_eval, tt_pv))) {
        // adjust checkmate scores according to our ply
        if (tt_score > MATE_THRESHOLD) {
            tt_score -= ply;
        } else if (tt_score < -MATE_THRESHOLD) {
            tt_score += ply;
        }

        // return the score from the TT if the bounds and depth allow it
        if (!pv_node && !is_root && tt_depth >= depth &&
            (node_type == EXACT_NODE || (node_type == LOWER_BOUND_NODE && tt_score >= beta) ||
             (node_type == UPPER_BOUND_NODE && tt_score <= alpha))) {
            return tt_score;
        }
    }

    // get static evaluation for use in pruning heuristics if we didn't get it from the tt
    if (!tt_hit) {
        s_eval = eval(pos);
    }
    // update the stack (used for the improving heuristic)
    search_stack[ply].s_eval = s_eval;

    /*
    Improving Heuristic

    We prune less when our static eval is improved from 2 plies ago
    */
    bool improving = ply < 2 || s_eval > search_stack[ply - 2].s_eval;

    /*
    Reverse Futility Pruning

    If our eval is above beta + some margin we consider this a beta cutoff
    */
    if (!pv_node && !in_check && depth <= 6 && s_eval >= beta + 120 * depth) {
        return s_eval;
    }

    /*
    Null Move Pruning

    This takes advantage of the observation that playing a move is nearly
    always better than doing nothing. We disable it in positions where
    zugzwang is likely (in this case king pawn endgames)
    */
    if (!pv_node && allow_nmp && depth >= 2 && !in_check && s_eval >= beta &&
        pos.zugzwangUnlikely()) {
        // calculate depth reduction
        int reduction = 3 + depth / 3;
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
    // allow_nmp = true;
    // recursive null move pruning disabled

    /*
    Check Extension
    */
    if (in_check && depth <= 3) depth++;

    /*
    Internal Iterative Reductions

    We reduce when the position is not in the transposition table,
    assuming that must mean the position is not very important.

    Using tt_move instead of tt_hit is a bit weird but it's what
    worked for me. Maybe I will change this at some point.
    */
    if (depth >= 5 && tt_move == NULL_MOVE && !in_check) depth--;

    // initialize move picker with appropriate data for move ordering
    MovePicker move_picker(pos, &quiet_history, tt_move, killer_1[ply], killer_2[ply]);

    move16 move = NULL_MOVE;
    int best_score = NEGATIVE_INFINITY;
    move16 best_move = NULL_MOVE;
    int num_moves = 0;
    bool is_upper_bound = true;
    bool skip_quiets = false;

    // List of all the quiet moves that didn't cause a beta cutoff. used for updating history
    MoveList bad_quiets;

    // reset the killer moves for the child nodes
    killer_1[ply + 1] = NULL_MOVE;
    killer_2[ply + 1] = NULL_MOVE;

    // enable or disable futility pruning
    bool allow_fprune = !pv_node && depth <= 4 && !in_check && s_eval < alpha - 50 - 80 * depth;

    /*
    Main Move Loop

    Loop through all the legal moves, or only noisy moves, depending on if futility pruning
    or late move pruning has been activated
    */
    while (true) {
        /*
        Futility pruning

        If our eval is far below alpha at a low depth, then after the first move search only
        moves with a decent chance of raising alpha. (in this case only captures and promotions)
        */
        if (allow_fprune && !skip_quiets && best_score > -MATE_THRESHOLD) skip_quiets = true;

        // get the next move from the move picker
        skip_quiets ? move = move_picker.getNextCapture() : move = move_picker.getNextMove();
        // break if there is no next move
        if (!move) break;

        /*
        SEE pruning

        at low depths prune moves determined as losing by the static exchange evaluator
        */
        if (best_score > -MATE_THRESHOLD && !in_check && depth <= 7 &&
            !seeGe(pos, move, -50 - 150 * !isQuiet(move) - 100 * improving))
            continue;

        // update the search stack
        search_stack[ply].move = move;
        search_stack[ply].piece_moved = pos.at(getFromSquare(move));

        pos.makeMove(move);

        // TT prefetching. avoids cache misses that cause slow TT lookups
        tt->prefetch(pos.z_key);

        /*
        Late Move Pruning

        prune late-ordered quiet moves in non-PV nodes at low depth
        */
        if (!pv_node && !skip_quiets && !in_check && depth <= 7 &&
            static_cast<int>(bad_quiets.size()) > 1 + depth * 2 + 3 * improving && isQuiet(move) &&
            !inCheck(pos)) {
            pos.unmakeMove();
            skip_quiets = true;
            continue;
        }

        // increase the move counter only for moves that aren't pruned
        num_moves++;
        int score = 0;

        /*
        Late Move Reductions

        Search later moves with a reduced depth. If they raise alpha then we
        search again with full depth. This is based on the assumption that
        moves ordered later by our move ordering scheme are less likely to
        be good
        */
        bool do_full_search = true;
        if (bad_quiets.size() > 1 && depth > 2 && !in_check &&
            (!pv_node || !isCaptureOrPromotion(move))) {
            // get pre-calculated reduction from the table
            int lmr_reduction = lmr_table[depth][num_moves];

            lmr_reduction -= tt_pv;

            lmr_reduction -= inCheck(pos);

            lmr_reduction -= isCaptureOrPromotion(move);

            // don't drop directly into qsearch
            lmr_reduction = std::min(lmr_reduction, depth - 1);

            // don't bother unless we have an actual reduction
            if (lmr_reduction > 1) {
                score = -negaMax<false, true, false>(pos, depth - lmr_reduction, ply + 1,
                                                     -alpha - 1, -alpha);

                // re-search when we raise alpha
                if (score > alpha) {
                    do_full_search = true;
                } else {
                    do_full_search = false;
                }
            }
        }

        /*
        Principal Variation Search

        The root node is a PV node and from there we search the first child
        of a PV node as a PV node with a full window and subsequent children as non-PV
        nodes with a zero window. If a move raises alpha in a pv node then we re-search
        that child as a pv node with a full window.

        The idea is that by searching most nodes with a zero window we increase cutoffs
        and search fewer total nodes even if re-searches are sometimes necessary.

        template parameters are <pv_node, cut_node, is_root>
        */
        if (pv_node && num_moves == 1) {
            // always search the first move of a pv node with a full window
            score = -negaMax<pv_node, false, false>(pos, depth - 1, ply + 1, -beta, -alpha);
        } else if (do_full_search) {
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
            if constexpr (pv_node) pv.updatePV(ply, move);
            // check for a beta cutoff
            if (score >= beta) {
                if (isQuiet(move)) {
                    // save this move as a killer move
                    saveKiller(ply, move);
                    // update butterfly history
                    int bonus = depth * depth;
                    updateHistory(pos.side_to_move, getFromSquare(move), getToSquare(move), bonus);
                    // apply malus to the previous quiet moves
                    for (auto &bq : bad_quiets) {
                        updateHistory(pos.side_to_move, getFromSquare(bq.move),
                                      getToSquare(bq.move), -bonus);
                    }
                }
                // Store to TT as a fail high
                tt->save(pos.z_key, depth, ply, move, score, LOWER_BOUND_NODE, s_eval, pv_node);
                // beta cutoff
                return score;
            } else if (score > alpha) {
                // raise alpha if necessary
                alpha = score;
                is_upper_bound = false;
            }
        }

        if (isQuiet(move)) bad_quiets.addMove(move);
    }

    // check for checkmate and stalemate
    if (num_moves == 0 && !move_picker.getNextMove()) {
        if (inCheck(pos)) {
            // subtract our current ply from the mate score to encourage faster checkmates
            return -MATE_SCORE + ply;
        }
        return 0;
    }

    // save to TT as an upper bound node or an exact node depending on if we raised alpha
    if (is_upper_bound) {
        // re-use the old TT move in fail lows
        tt->save(pos.z_key, depth, ply, tt_move, best_score, UPPER_BOUND_NODE, s_eval, pv_node);
    } else {
        tt->save(pos.z_key, depth, ply, best_move, best_score, EXACT_NODE, s_eval, pv_node);
    }

    return best_score;
}

// quiescence search
int Search::qSearch(Position &pos, int depth, int ply, int alpha, int beta) {
    if (timesUp() || pos.fifty_move >= 100 || pos.isTripleRepetition()) {
        return 0;
    }
    // check ply limit
    if (ply >= MAX_PLY - 1) return eval(pos);

    pv.zeroLength(ply);
    nodes_searched++;
    q_nodes++;

    bool in_check = inCheck(pos);

    move16 tt_move = NULL_MOVE;
    bool tt_hit = false;
    NodeType node_type;
    int tt_depth;
    int tt_score;
    bool tt_pv;
    // our quiescence search static eval. used as a lower bound for our score
    int stand_pat;

    // Probe the transposition table
    if ((tt_hit = tt->probe(pos.z_key, tt_move, node_type, tt_depth, tt_score, stand_pat, tt_pv))) {
        // adjust checkmate scores according to our ply
        if (tt_score > MATE_THRESHOLD) {
            tt_score -= ply;
        } else if (tt_score < -MATE_THRESHOLD) {
            tt_score += ply;
        }

        // return the score from the TT if the bounds and depth allow it
        if (enable_qsearch_tt && tt_depth >= depth &&
            (node_type == EXACT_NODE || (node_type == LOWER_BOUND_NODE && tt_score >= beta) ||
             (node_type == UPPER_BOUND_NODE && tt_score <= alpha))) {
            return tt_score;
        }
    }

    // don't use standing pat when in check. (we need to search evasions)
    if (in_check) {
        stand_pat = NEGATIVE_INFINITY;
    } else if (!tt_hit) {
        stand_pat = eval(pos);
    }

    bool is_upper_bound = true;

    if (stand_pat >= beta) {
        // standing pat beta cutoff
        return stand_pat;
    } else if (stand_pat > alpha) {
        /*
        Use the static eval as a lower bound for our score.
        This is sound because in chess making a move is
        usually better than doing nothing.
        */
        alpha = stand_pat;
        is_upper_bound = false;
    }

    MovePicker move_picker(pos, &quiet_history, tt_move, 0, 0);

    move16 move = NULL_MOVE;
    move16 best_move = NULL_MOVE;
    int best_score = stand_pat;
    int num_moves = 0;

    // if in check search all legal moves, otherwise search captures and promotions
    while (in_check ? move = move_picker.getNextMove() : move = move_picker.getNextCapture()) {
        num_moves++;

        /*
        Delta Pruning / SEE pruning

        use SEE to determine good/bad captures and skip the ones with little chance
        of raising alpha
        */
        if (!in_check && !isQuiet(move) && !seeGe(pos, move, (alpha - stand_pat) - SEE_MARGIN))
            continue;

        pos.makeMove(move);
        int score = -qSearch(pos, depth - 1, ply + 1, -beta, -alpha);
        pos.unmakeMove();

        if (score > best_score) {
            best_score = score;
            best_move = move;
            if (score >= beta) {
                tt->save(pos.z_key, depth, ply, move, score, LOWER_BOUND_NODE, stand_pat, false);
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

    if (is_upper_bound) {
        tt->save(pos.z_key, depth, ply, tt_move, best_score, UPPER_BOUND_NODE, stand_pat, false);
    } else {
        tt->save(pos.z_key, depth, ply, best_move, best_score, EXACT_NODE, stand_pat, false);
    }

    return best_score;
}

int Search::qScore(Position &pos) {
    setTimer(1000, 1000);
    node_search = false;
    enable_qsearch_tt = false;

    return qSearch(pos, 0, 0, NEGATIVE_INFINITY, POSITIVE_INFINITY);
}

}  // namespace Spotlight
