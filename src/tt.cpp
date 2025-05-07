#include "tt.hpp"

namespace Spotlight {

TTEntry::TTEntry() : 
z_key(0ULL), depth(0), best_move(0), score(0), node_type(NULL_NODE), half_moves(0)
{
    
}

TTEntry::TTEntry(U64 _z_key, int _depth, move16 _best_move, int _score, uint8_t _node_type, int _half_moves, int _s_eval) : 
z_key(_z_key), depth(_depth), best_move(_best_move), score(_score), node_type(_node_type), half_moves(_half_moves),
s_eval(_s_eval)
{

}

TT::TT(): num_entries(NUM_ENTRIES), hash_size(TT_SIZE) {
    hash_table.resize(NUM_ENTRIES);
    clear();
}

TT::TT(size_t size): hash_size(size), num_entries(size / sizeof(TTEntry)) {
    hash_table.resize(num_entries);
    clear();
}

void TT::resize(size_t size) {
    hash_size = size;
    num_entries = size / sizeof(TTEntry);
    hash_table.resize(num_entries);
}

void TT::clear() {
    for (int i = 0; i < num_entries; i++) {
        hash_table[i] = TTEntry();
    }
}


void TT::save(U64 z_key, int depth, int ply, move16 best_move, int score, uint8_t node_type, int half_moves, int s_eval) {
    TTEntry* old_entry = probe(z_key);

    if (old_entry->depth > depth && old_entry->z_key == z_key) {
        return;
    }

    if (score > MATE_THRESHOLD) {
        score += ply;
    } else if (score < -MATE_THRESHOLD) {
        score -= ply;
    }

    *old_entry = TTEntry(z_key, depth, best_move, score, node_type, half_moves, s_eval);
}

} // namespace Spotlight
