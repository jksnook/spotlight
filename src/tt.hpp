#pragma once

#include <vector>

#include "move.hpp"
#include "types.hpp"

namespace Spotlight {

const size_t TT_SIZE = 1024 * 1024 * 16;
const int MAX_PLY = 100;
const int MATE_SCORE = 30000;
const int MATE_THRESHOLD = MATE_SCORE - MAX_PLY;
const int BUCKET_SIZE = 3;
enum NodeType : uint8_t { NULL_NODE, EXACT_NODE, LOWER_BOUND_NODE, UPPER_BOUND_NODE };

class TTEntry {
   public:
    TTEntry();
    TTEntry(U64 _z_key, int _depth, move16 _best_move, int _score, NodeType _node_type, int _s_eval,
            uint8_t _age, bool _is_pv);

    inline NodeType getNodeType() { return static_cast<NodeType>(flags & 0b11); }

    inline bool getIsPV() { return static_cast<bool>(flags >> 2 & 1); }

    uint16_t hash16;
    int16_t depth;
    move16 best_move;
    int16_t score;
    int16_t s_eval;
    uint8_t flags;
    uint8_t age;

   private:
};

struct TTBucket {
    TTEntry entries[BUCKET_SIZE];
};

const int NUM_ENTRIES = TT_SIZE / sizeof(TTBucket);

class TT {
   public:
    TT();
    TT(size_t size);
    ~TT(){};

    void resize(size_t size);
    void clear();
    void nextGeneration();
    bool probe(U64 z_key, move16 &tt_move, NodeType &node_type, int &depth, int &score, int &s_eval,
               bool &tt_pv);
    void save(U64 z_key, int depth, int ply, move16 best_move, int score, NodeType node_type,
              int s_eval, bool is_pv);
    void prefetch(U64 z_key);
    int hashfull();

   private:
    size_t hash_size;
    U64 num_entries;
    std::vector<TTBucket> hash_table;
    uint8_t generation;
};

}  // namespace Spotlight
