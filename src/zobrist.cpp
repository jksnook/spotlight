#include "zobrist.hpp"

#include <random>

#include "types.hpp"

namespace Spotlight {

U64 piece_keys[static_cast<int>(Piece::NO_PIECE) + 1][64];
U64 en_passant_keys[64];
U64 castle_rights_keys[16];
U64 side_key;

void initZobrist() {
    std::mt19937_64 randomU64(15);

    for (int i = 0; i < 64; i++) {
        en_passant_keys[i] = randomU64();

        for (int piece = 0; piece < static_cast<int>(Piece::NO_PIECE);
             piece++) {
            piece_keys[piece][i] = randomU64();
        }

        piece_keys[static_cast<int>(Piece::NO_PIECE)][i] = 0ULL;
    }

    en_passant_keys[0] = 0ULL;

    for (int i = 0; i < 16; i++) {
        castle_rights_keys[i] = randomU64();
    }

    side_key = randomU64();
}

}  // namespace Spotlight
