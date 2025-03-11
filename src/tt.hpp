#pragma once

#include "types.hpp"
#include "move.hpp"

#include <vector>

const int TT_SIZE = 1024 * 1024 * 16;
const int MAX_DEPTH = 100;
const int MATE_SCORE = 1 << 15;
const int MATE_THRESHOLD = MATE_SCORE - MAX_DEPTH;
const int NULL_NODE = 0;
const int EXACT_NODE = 1;
const int LOWER_BOUND_NODE = 2;
const int UPPER_BOUND_NODE = 3;

class TTEntry {
    public:
        TTEntry();
        TTEntry(U64 _z_key, int _depth, move16 _best_move, int _score, int _node_type, int _half_moves);

        U64 z_key;
        int16_t depth;
        move16 best_move;
        int score;
        uint8_t node_type;
        uint16_t half_moves;
    private:
};

const int NUM_ENTRIES = TT_SIZE / sizeof(TTEntry);

class TT {
    public:
        TT();
        ~TT() {};

        void clear();
        void save(U64 z_key, int depth, int ply, move16 best_move, int score, int node_type, int half_moves);
        inline TTEntry getEntry(U64 z_key) {return hash_table[z_key % NUM_ENTRIES];};
        TTEntry* probe(U64 z_key);
        bool getScore(U64 z_key, int depth, int ply, int alpha, int beta, int &score, move16 &best_move);
    private:
        std::vector<TTEntry> hash_table;

};