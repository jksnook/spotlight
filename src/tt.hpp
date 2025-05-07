#pragma once

#include "types.hpp"
#include "move.hpp"

#include <vector>

namespace Spotlight {

const size_t TT_SIZE = 1024 * 1024 * 16;
const int MAX_PLY = 100;
const int MATE_SCORE = 1 << 15;
const int MATE_THRESHOLD = MATE_SCORE - MAX_PLY;
const uint8_t NULL_NODE = 0;
const uint8_t EXACT_NODE = 1;
const uint8_t LOWER_BOUND_NODE = 2;
const uint8_t UPPER_BOUND_NODE = 3;

class TTEntry {
public:
    TTEntry();
    TTEntry(U64 _z_key, int _depth, move16 _best_move, int _score, uint8_t _node_type, int _half_moves, int _s_eval);

    U64 z_key;
    int16_t depth;
    move16 best_move;
    int score;
    uint8_t node_type;
    uint16_t half_moves;
    int32_t s_eval;
private:
};

const int NUM_ENTRIES = TT_SIZE / sizeof(TTEntry);

class TT {
public:
    TT();
    TT(size_t size);
    ~TT() {};

    void resize(size_t size);
    void clear();
    void save(U64 z_key, int depth, int ply, move16 best_move, int score, uint8_t node_type, int half_moves, int s_eval);
    inline TTEntry getEntry(U64 z_key) {return hash_table[z_key % num_entries];};
    inline TTEntry* probe(U64 z_key) {return &hash_table[z_key % num_entries];};
private:
    U64 num_entries;
    size_t hash_size;
    std::vector<TTEntry> hash_table;
};

}