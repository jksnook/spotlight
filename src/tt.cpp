#include "tt.hpp"

TTEntry::TTEntry() : 
z_key(0ULL), depth(0), best_move(0), score(0), node_type(NULL_NODE), half_moves(0)
{
    
}

TTEntry::TTEntry(U64 _z_key, int _depth, move16 _best_move, int _score, int _node_type, int _half_moves) : 
z_key(_z_key), depth(_depth), best_move(_best_move), score(_score), node_type(_node_type), half_moves(_half_moves)
{

}

TT::TT() {
    hash_table.resize(NUM_ENTRIES);
    clear();
}

void TT::clear() {
    for (int i = 0; i < NUM_ENTRIES; i++) {
        hash_table[i] = TTEntry();
    }
}


void TT::save(U64 z_key, int depth, int ply, move16 best_move, int score, int node_type, int half_moves) {
    TTEntry* old_entry = probe(z_key);

    if (old_entry->depth > depth && old_entry->z_key == z_key) {
        return;
    }

    if (score > MATE_THRESHOLD) {
        score += ply;
    } else if (score < -MATE_THRESHOLD) {
        score -= ply;
    }

    *old_entry = TTEntry(z_key, depth, best_move, score, node_type, half_moves);
}

TTEntry* TT::probe(U64 z_key) {
    return &hash_table[z_key % NUM_ENTRIES];
}

bool TT::getScore(U64 z_key, int depth, int ply, int alpha, int beta, int &score, move16 &best_move) {
    TTEntry* entry = probe(z_key);

    if (entry->z_key != z_key || entry->node_type == 0) {
        return false;
    }

    assert(z_key == entry->z_key);

    best_move = entry->best_move;

    if (entry->depth < depth) {
        return false;
    }

    score = entry->score;

    if (score > MATE_THRESHOLD) {
        score -= ply;
    } else if (score < -MATE_THRESHOLD) {
        score += ply;
    }

    switch (entry->node_type)
    {
    case UPPER_BOUND_NODE:
        if (score > alpha) {
            score = 0;
            return false;
        } else {
            return true;
        }
        break;
    case LOWER_BOUND_NODE:
        if (score >= beta) {
            return true;
        } else {
            score = 0;
            return false;
        }
        break;
    case EXACT_NODE:
        return true;
        break;
    default:
        break;
    }

    return true;
}