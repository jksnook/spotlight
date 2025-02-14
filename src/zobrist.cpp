#include "zobrist.hpp"
#include "bitboards.hpp"

U64 piece_keys[NO_PIECE + 1][64];
U64 en_passant_keys[64];
U64 castle_rights_keys[16];
U64 side_key;

void initZobrist() {
    for (int i = 0; i < 64; i++) {
        en_passant_keys[i] = randomU64();

        for (int piece = white_pawn; piece <= black_king; piece++) {
            piece_keys[piece][i] = randomU64();
        }
        piece_keys[NO_PIECE][i] = 0ULL;
    }

    en_passant_keys[0] = 0ULL;

    for (int i = 0; i < 16; i++) {
        castle_rights_keys[i] = randomU64();
    }

    // for (int i = 0; i < 8; i++) {
    //   en_passant_keys[i] = randomU64();
    // }

    side_key = randomU64();
}
