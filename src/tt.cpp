#include "tt.hpp"

namespace Spotlight {

TTEntry::TTEntry() : 
hash16(0ULL), depth(0), best_move(0), score(0), node_type(NULL_NODE)
{
    
}

TTEntry::TTEntry(U64 _z_key, int _depth, move16 _best_move, int _score, NodeType _node_type, int _s_eval, uint8_t _age) : 
depth(_depth), best_move(_best_move), score(_score), node_type(_node_type), age(_age),
s_eval(_s_eval)
{
    hash16 = static_cast<uint16_t>(_z_key >> 48);
}

TT::TT(): num_entries(NUM_ENTRIES), hash_size(TT_SIZE) {
    hash_table.resize(NUM_ENTRIES);
    clear();
}

TT::TT(size_t size): hash_size(size), num_entries(size / sizeof(TTBucket)), generation(0) {
    hash_table.resize(num_entries);
    clear();
}

void TT::resize(size_t size) {
    hash_size = size;
    num_entries = size / sizeof(TTBucket);
    hash_table.resize(num_entries);
}

void TT::clear() {
    for (auto &bucket: hash_table) {
        for (auto &entry: bucket.entries) {
            entry = TTEntry();
        }
    }
    generation = 0;
}

void TT::nextGeneration() {
    generation++;
}

// Fetches data from the TT. Returns true if there is a matching hash.
bool TT::probe(U64 z_key, move16 &tt_move, NodeType &node_type, int &depth, int &score, int &s_eval) {
    uint16_t hash16 = static_cast<uint16_t>(z_key >> 48);

    TTBucket* bucket = &hash_table[z_key % num_entries];

    for (auto &entry: bucket->entries) {
        if (entry.hash16 == hash16) {
            tt_move = entry.best_move;
            node_type = entry.node_type;
            depth = entry.depth;
            score = entry.score;
            s_eval = entry.s_eval;
            return true;
        }
    }
    return false;
}

// saves data to the TT
void TT::save(U64 z_key, int depth, int ply, move16 best_move, int score, NodeType node_type, int s_eval) {
    int16_t worst_score = 32000;
    uint16_t hash16 = static_cast<uint16_t>(z_key >> 48);
    TTBucket* bucket = &hash_table[z_key % num_entries];
    TTEntry* to_replace = &bucket->entries[0];

    // adjust checkmate scores
    if (score > MATE_THRESHOLD) {
        score += ply;
    } else if (score < -MATE_THRESHOLD) {
        score -= ply;
    }

    // look for a matching hash, if not then get the one with the
    // lowest replacement score
    //
    // replacement score is depth - relative age * 8
    for (auto &entry: bucket->entries) {
        if (entry.hash16 == hash16) {
            to_replace = &entry;
            break;
        }
        int replacement_score = entry.depth - (generation - entry.age) * 8;
        if (replacement_score < worst_score) {
            worst_score = replacement_score;
            to_replace = &entry;
        }
    }

    // only replace a matching entry if the new depth is greater or
    // the new node type is exact
    if (to_replace->hash16 == hash16) {
        if (depth >= to_replace->depth || node_type == EXACT_NODE) {
            *to_replace = TTEntry(z_key, depth, best_move, score, node_type, s_eval, generation);
        }
        return;
    }

    *to_replace = TTEntry(z_key, depth, best_move, score, node_type, s_eval, generation);
}

void TT::prefetch(U64 z_key) {
    __builtin_prefetch(&hash_table[z_key % num_entries]);
}

} // namespace Spotlight
