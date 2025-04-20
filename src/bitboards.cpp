#include "bitboards.hpp"
#include <iostream>

// Reverse Bitscan from the chess programming wiki
/**
 * bitScanReverse
 * @authors Kim Walisch, Mark Dickinson
 * @param bitboard bitboard to scan
 * @precondition bitboard != 0
 * @return index (0..63) of most significant one bit
 */
int bitScanReverse(U64 bitboard) {
    // #if defined(__GNUC__) ||  defined(__GNUG__)

    // return __builtin_clzll(bitboard);

    // #else

    assert(bitboard != 0);
    static constexpr U64 debruijn64 = 0x03f79d71b4cb0a89ULL;
    bitboard |= bitboard >> 1; 
    bitboard |= bitboard >> 2;
    bitboard |= bitboard >> 4;
    bitboard |= bitboard >> 8;
    bitboard |= bitboard >> 16;
    bitboard |= bitboard >> 32;
    return index64reverse[(bitboard * debruijn64) >> 58];

    //  #endif
}

void printBitboard(U64 bitboard) 
{
    std::string bitboard_string = "";
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
        if ((bitboard >> (rank * 8 + file)) & 1) {
            bitboard_string += "1";
        } else {
            bitboard_string += "0";
        };
        };
        bitboard_string += "\n";
    };
    std::cout << bitboard_string << std::endl;
}

U64 pawn_pushes[2][64];
U64 pawn_double_pushes[2][64];
U64 pawn_attacks[2][64];

U64 knight_moves[64];
U64 king_moves[64];

U64 sliding_moves[8][64];

U64 bishop_moves[64];
U64 rook_moves[64];

U64 bishop_magic_mask[64];
U64 rook_magic_mask[64];

U64 rook_relevant_bit_count[64];
U64 bishop_relevant_bit_count[64];

U64 generateWhitePawnPush(int index) {
    return (1ULL << (index + 8)) & NOT_RANK_1;
}

U64 generateWhitePawnDoublePush(int index) {
    if (1ULL << index & RANK_2) {
        return (1ULL << (index + 16));
    }
    return 0ULL;
}

U64 generateWhitePawnAttack(int index) {
    return (1ULL << (index + 7) & NOT_H_FILE) | (1ULL << (index + 9) & NOT_A_FILE);
}

U64 generateBlackPawnPush(int index) {
    return (1ULL << (index - 8)) & NOT_RANK_8;
}

U64 generateBlackPawnDoublePush(int index) {
    if (1ULL << index & RANK_7) {
        return (1ULL << (index - 16));
    }
    return 0ULL;
    }

U64 generateBlackPawnAttack(int index) {
    return (1ULL << (index - 7) & NOT_A_FILE) | (1ULL << (index - 9) & NOT_H_FILE);
}

U64 generateKnightMove(int index) {
    U64 knight_square = 1ULL << index;
    U64 moves = 0ULL;
    moves |= (knight_square << 17) & NOT_A_FILE;
    moves |= (knight_square <<  15) & NOT_H_FILE;
    moves |= (knight_square <<  10) & NOT_AB_FILE;
    moves |= (knight_square <<  6) & NOT_GH_FILE;
    moves |= (knight_square >> 6) & NOT_AB_FILE;
    moves |= (knight_square >> 10) & NOT_GH_FILE;
    moves |= (knight_square >> 15) & NOT_A_FILE;
    moves |= (knight_square >> 17) & NOT_H_FILE;
    return moves;
}

U64 generateKingMove(int index) {
    U64 king_square = 1ULL << index;
    U64 moves = 0ULL;
    moves |= (king_square << 9) & NOT_A_FILE;
    moves |= (king_square << 8);
    moves |= (king_square << 7) & NOT_H_FILE;
    moves |= (king_square << 1) & NOT_A_FILE;
    moves |= (king_square >> 1) & NOT_H_FILE;
    moves |= (king_square >> 7) & NOT_A_FILE;
    moves |= (king_square >> 8);
    moves |= (king_square >> 9) & NOT_H_FILE;

    return moves;
}

U64 generateSlidingMove_NW(int index) {
    U64 moves = 0ULL;
    int rank, file;
    for (rank = index / 8 + 1, file = index % 8 - 1; rank < 8 && file >= 0; rank++, file--) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

U64 generateSlidingMove_N(int index) {
    U64 moves = 0ULL;
    int file = index % 8;
    for (int rank = index / 8 + 1; rank < 8; rank++ ) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

U64 generateSlidingMove_NE(int index) {
    U64 moves = 0ULL;
    int rank, file;
    for (rank = index / 8 + 1, file = index % 8 + 1; rank < 8 && file < 8; rank++, file++) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

U64 generateSlidingMove_E(int index) {
    U64 moves = 0ULL;
    int rank = index / 8;
    for (int file = index % 8 + 1; file < 8; file++ ) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

U64 generateSlidingMove_SE(int index) {
    U64 moves = 0ULL;
    int rank, file;
    for (rank = index / 8 - 1, file = index % 8 + 1; rank >= 0 && file < 8; rank--, file++) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

U64 generateSlidingMove_S(int index) {
U64 moves = 0ULL;
int file = index % 8;
for (int rank = index / 8 - 1; rank >= 0; rank-- ) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

U64 generateSlidingMove_SW(int index) {
U64 moves = 0ULL;
int rank, file;
for (rank = index / 8 - 1, file = index % 8 - 1; rank >= 0 && file >=0; rank--, file--) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

U64 generateSlidingMove_W(int index) {
U64 moves = 0ULL;
int rank = index / 8;
for (int file = index % 8 - 1; file >= 0; file-- ) {
        moves |= 1ULL << (rank * 8 + file);
    }
    return moves;
}

void initMoves() {
for (int i = 0; i < 64; i++) {
    pawn_pushes[0][i] = generateWhitePawnPush(i);
    pawn_double_pushes[0][i] = generateWhitePawnDoublePush(i);
    pawn_attacks[0][i] = generateWhitePawnAttack(i);

    pawn_pushes[1][i] = generateBlackPawnPush(i);
    pawn_double_pushes[1][i] = generateBlackPawnDoublePush(i);
    pawn_attacks[1][i] = generateBlackPawnAttack(i);

    knight_moves[i] = generateKnightMove(i);
    king_moves[i] = generateKingMove(i);

    sliding_moves[0][i] = generateSlidingMove_NW(i);
    sliding_moves[1][i] = generateSlidingMove_N(i);
    sliding_moves[2][i] = generateSlidingMove_NE(i);
    sliding_moves[3][i] = generateSlidingMove_E(i);
    sliding_moves[4][i] = generateSlidingMove_SE(i);
    sliding_moves[5][i] = generateSlidingMove_S(i);
    sliding_moves[6][i] = generateSlidingMove_SW(i);
    sliding_moves[7][i] = generateSlidingMove_W(i);

    bishop_moves[i] = sliding_moves[0][i] | sliding_moves[2][i] | sliding_moves[4][i] | sliding_moves[6][i];
    rook_moves[i] = sliding_moves[1][i] | sliding_moves[3][i] | sliding_moves[5][i] | sliding_moves[7][i];

    bishop_magic_mask[i] = bishop_moves[i] & NOT_A_FILE & NOT_H_FILE & NOT_RANK_1 & NOT_RANK_8;
    rook_magic_mask[i] = rook_moves[i];

    U64 this_square = 1ULL << i;

    if (this_square & NOT_A_FILE) rook_magic_mask[i] &= NOT_A_FILE;
    if (this_square & NOT_H_FILE) rook_magic_mask[i] &= NOT_H_FILE;
    if (this_square & NOT_RANK_1) rook_magic_mask[i] &= NOT_RANK_1;
    if (this_square & NOT_RANK_8) rook_magic_mask[i] &= NOT_RANK_8;

    bishop_relevant_bit_count[i] = countBits(bishop_magic_mask[i]);
    rook_relevant_bit_count[i] = countBits(rook_magic_mask[i]);
}
}

U64 generatePositiveRayAttack(int direction, int index, U64 occcupancy) {
    U64 attack_ray = sliding_moves[direction][index];
    U64 blockers = attack_ray & occcupancy;
    if (blockers) {
        attack_ray &= ~sliding_moves[direction][bitScanForward(blockers)];
    }
    return attack_ray;
}

U64 generateNegativeRayAttack(int direction, int index, U64 occcupancy) {
U64 attack_ray = sliding_moves[direction][index];
U64 blockers = attack_ray & occcupancy;
if (blockers) {
    attack_ray &= ~sliding_moves[direction][bitScanReverse(blockers)];
}
return attack_ray;
}

U64 generateBishopAttacks(int index, U64 occupancy) {
    U64 attacks = 0ULL;
    attacks |= generatePositiveRayAttack(0, index, occupancy);
    attacks |= generatePositiveRayAttack(2, index, occupancy);
    attacks |= generateNegativeRayAttack(4, index, occupancy);
    attacks |= generateNegativeRayAttack(6, index, occupancy);

    return attacks;
}

U64 generateRookAttacks(int index, U64 occupancy) {
    U64 attacks = 0ULL;
    attacks |= generatePositiveRayAttack(1, index, occupancy);
    attacks |= generatePositiveRayAttack(3, index, occupancy);
    attacks |= generateNegativeRayAttack(5, index, occupancy);
    attacks |= generateNegativeRayAttack(7, index, occupancy);

    return attacks;
}

U64 rook_magic_attacks[64][4096];
U64 bishop_magic_attacks[64][1024];

// iterate to the next occupancy as if counting bit by bit with the bits from the mask (still not ideal. looking for something better.)
void iterateOccupancy(U64 &occupancy, U64 mask) {
    U64 least_sig_zero = (occupancy ^ mask) & -(occupancy ^ mask);
    occupancy = (occupancy & ~((least_sig_zero - 1))) | (least_sig_zero);
}

std::mt19937_64 randomU64(50);

// find a working magic number at the given index and populate the arrays
// Currently just loads a saved number to save startup time
void findBishopMagic(int index) {
    U64 magic_number;
    U64 all_bits = ~0ULL >> (64 - bishop_relevant_bit_count[index]);
    U64 occupancy_bits;
    U64 occupancy;
    U64 attacks;
    U64 magic_index;
    U64 mask = bishop_magic_mask[index];

    magic_number = bishop_magic_numbers[index];

    //reset all the attacks to zero each time
    for (int i = 0; i < 1024; i++) {
        bishop_magic_attacks[index][i] = 0ULL;
    }

    occupancy = 0ULL;
    for (occupancy_bits = 0ULL; occupancy_bits <= all_bits; occupancy_bits++) {
        iterateOccupancy(occupancy, mask);
        attacks = generateBishopAttacks(index, occupancy);
        magic_index = (occupancy * magic_number) >> (64 - (bishop_relevant_bit_count[index]));
        //reliability check for magic number
        assert(bishop_magic_attacks[index][magic_index] == attacks || bishop_magic_attacks[index][magic_index] == 0ULL);
        bishop_magic_attacks[index][magic_index] = attacks;
    }
}

// find a working magic number at the given index and populate the arrays
// Currently just loads a saved number to save startup time
void findRookMagic(int index) {
    U64 magic_number;
    U64 all_bits = ~0ULL >> (64 - rook_relevant_bit_count[index]);
    U64 occupancy_bits;
    U64 occupancy;
    U64 attacks;
    U64 magic_index;
    U64 mask = rook_magic_mask[index];


    //get a new magic number
    magic_number = rook_magic_numbers[index];

    //reset all attack masks to zero
    for (int i = 0; i < 4096; i++) {
        rook_magic_attacks[index][i] = 0ULL;
    }

    occupancy = 0ULL;
    // loop over all possible occupancies
    for (occupancy_bits = 0ULL; occupancy_bits <= all_bits; occupancy_bits++) {
        iterateOccupancy(occupancy, mask);
        attacks = generateRookAttacks(index, occupancy);
        magic_index = (magic_number * occupancy) >> (64 - rook_relevant_bit_count[index]);
        //reliability check for magic numbers
        assert(rook_magic_attacks[index][magic_index] == attacks || rook_magic_attacks[index][magic_index] == 0);
        rook_magic_attacks[index][magic_index] = attacks;
    }
}

void initMagics() {
    for (int i = 0; i < 64; i++) {
        findBishopMagic(i);
        findRookMagic(i);
    }
}

U64 knightAttacksFromBitboard(U64 bitboard) {
    U64 attacks;
    attacks = (bitboard << 17) & NOT_A_FILE;
    attacks |= (bitboard <<  15) & NOT_H_FILE;
    attacks |= (bitboard <<  10) & NOT_AB_FILE;
    attacks |= (bitboard <<  6) & NOT_GH_FILE;
    attacks |= (bitboard >> 6) & NOT_AB_FILE;
    attacks |= (bitboard >> 10) & NOT_GH_FILE;
    attacks |= (bitboard >> 15) & NOT_A_FILE;
    attacks |= (bitboard >> 17) & NOT_H_FILE;
    return attacks;
}

U64 bishopAttacksFromBitboard(U64 bishops, U64 occupancy) {
  U64 attacks = 0ULL;
  while (bishops) {
    attacks |= getMagicBishopAttack(popLSB(bishops), occupancy);
  }
  return attacks;
}

U64 rookAttacksFromBitboard(U64 rooks, U64 occupancy) {
  U64 attacks = 0ULL;
  while (rooks) {
    attacks |= getMagicRookAttack(popLSB(rooks), occupancy);
  }
  return attacks;
}