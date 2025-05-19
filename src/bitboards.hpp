#pragma once

#include <cassert>
#include <random>
#include <string>

#include "types.hpp"
#include "utils.hpp"

namespace Spotlight {

// file masks
static constexpr BitBoard A_FILE = 0x0101010101010101ULL;
static constexpr BitBoard B_FILE = A_FILE << 1;
static constexpr BitBoard G_FILE = A_FILE << 6;
static constexpr BitBoard H_FILE = A_FILE << 7;
static constexpr BitBoard AB_FILE = A_FILE | B_FILE;
static constexpr BitBoard GH_FILE = G_FILE | H_FILE;
static constexpr BitBoard NOT_A_FILE = ~A_FILE;
static constexpr BitBoard NOT_B_FILE = ~B_FILE;
static constexpr BitBoard NOT_AB_FILE = ~AB_FILE;
static constexpr BitBoard NOT_G_FILE = ~G_FILE;
static constexpr BitBoard NOT_H_FILE = ~H_FILE;
static constexpr BitBoard NOT_GH_FILE = ~GH_FILE;

// rank masks
static constexpr BitBoard RANK_1 = 0xffULL;
static constexpr BitBoard RANK_2 = RANK_1 << 8;
static constexpr BitBoard RANK_4 = RANK_1 << (8 * 3);
static constexpr BitBoard RANK_5 = RANK_1 << (8 * 4);
static constexpr BitBoard RANK_7 = RANK_1 << (8 * 6);
static constexpr BitBoard RANK_8 = RANK_1 << (8 * 7);
static constexpr BitBoard RANK_1_AND_2 = RANK_1 | RANK_2;
static constexpr BitBoard RANK_7_AND_8 = RANK_7 | RANK_8;
static constexpr BitBoard NOT_RANK_1 = ~RANK_1;
static constexpr BitBoard NOR_RANK_2 = ~RANK_2;
static constexpr BitBoard NOT_RANK_7 = ~RANK_7;
static constexpr BitBoard NOT_RANK_8 = ~RANK_8;
static constexpr BitBoard NOT_RANK_1_AND_2 = ~RANK_1_AND_2;
static constexpr BitBoard NOT_RANK_7_AND_8 = ~RANK_7_AND_8;

// castle squares needed for move generation and validation
const BitBoard WQC_SQUARES = 0b1110ULL;
const BitBoard WQC_KING_SQUARES = 0b1100ULL;
const BitBoard WKC_SQUARES = 0b1100000ULL;
const BitBoard WKC_KING_SQUARES = 0b1100000ULL;
const BitBoard BQC_SQUARES = 0b1110ULL << (8 * 7);
const BitBoard BQC_KING_SQUARES = WQC_KING_SQUARES << (8 * 7);
const BitBoard BKC_SQUARES = 0b1100000ULL << (8 * 7);
const BitBoard BKC_KING_SQUARES = WKC_KING_SQUARES << (8 * 7);

inline constexpr BitBoard setBit(int bit) { return 1ULL << bit; }

inline constexpr BitBoard setBit(Square bit) { return 1ULL << bit; }

inline constexpr BitBoard lsb(BitBoard bb) { return bb & -bb; }

// clang-format off
const int index64[64] = {
    0,  1, 48,  2, 57, 49, 28,  3,
   61, 58, 50, 42, 38, 29, 17,  4,
   62, 55, 59, 36, 53, 51, 43, 22,
   45, 39, 33, 30, 24, 18, 12,  5,
   63, 47, 56, 27, 60, 41, 37, 16,
   54, 35, 52, 21, 44, 32, 23, 11,
   46, 26, 40, 15, 34, 20, 31, 10,
   25, 14, 19,  9, 13,  8,  7,  6
};
// clang-format on

// Bitscan from the chess programming wiki (use built in CPU instruction if possible)
/**
 * bitScanForward
 * @author Martin Läuter (1997)
 *         Charles E. Leiserson
 *         Harald Prokop
 *         Keith H. Randall
 * "Using de Bruijn Sequences to Index a 1 in a Computer Word"
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
static inline Square bitScanForward(BitBoard bitboard) {
    assert(bitboard != 0ULL);
#if defined(__GNUC__) || defined(__GNUG__)

    return static_cast<Square>(__builtin_ctzll(bitboard));

#else

    const BitBoard debruijn64 = 0x03f79d71b4cb0a89ULL;
    return index64[((bitboard & -bitboard) * debruijn64) >> 58];

#endif
}

static inline Square popLSB(BitBoard &bitboard) {
    int bit = bitScanForward(bitboard);
    bitboard &= ~(1ULL << bit);
    return static_cast<Square>(bit);
};

static inline int countBits(BitBoard bitboard) {
#if defined(__GNUC__) || defined(__GNUG__)

    return __builtin_popcountll(bitboard);

#else

    static constexpr BitBoard m1 = 0x5555555555555555ULL;
    static constexpr BitBoard m2 = 0x3333333333333333ULL;
    static constexpr BitBoard m3 = 0x0f0f0f0f0f0f0f0fULL;
    static constexpr BitBoard m4 = 0x00ff00ff00ff00ffULL;
    static constexpr BitBoard m5 = 0x0000ffff0000ffffULL;
    static constexpr BitBoard m6 = 0x00000000ffffffffULL;

    BitBoard count = (bitboard & m1) + ((bitboard & m1 << 1) >> 1);
    count = (count & m2) + ((count & m2 << 2) >> 2);
    count = (count & m3) + ((count & m3 << 4) >> 4);
    count = (count & m4) + ((count & m4 << 8) >> 8);
    count = (count & m5) + ((count & m5 << 16) >> 16);
    count = (count & m6) + ((count & m6 << 32) >> 32);
    return count;

#endif
}

// clang-format off
const int index64reverse[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};
// clang-format on

Square bitScanReverse(BitBoard bitboard);

void printBitboard(BitBoard bitboard);

// extern std::mt19937_64 randomU64;

extern BitBoard pawn_pushes[2][64];
extern BitBoard pawn_double_pushes[2][64];
extern BitBoard pawn_attacks[2][64];

extern BitBoard knight_moves[64];
extern BitBoard king_moves[64];

extern BitBoard sliding_moves[8][64];

extern BitBoard bishop_moves[64];
extern BitBoard rook_moves[64];

extern BitBoard bishop_magic_mask[64];
extern BitBoard rook_magic_mask[64];

extern BitBoard rook_relevant_bit_count[64];
extern BitBoard bishop_relevant_bit_count[64];

// saved magic numbers to reduce startup time
const BitBoard rook_magic_numbers[64] = {
    0x1480004000201080ULL, 0x40002000c81000ULL,   0x2100084411002000ULL, 0x2880080050020480ULL,
    0x280080180040002ULL,  0x420008a402001005ULL, 0x1080020020804100ULL, 0x1000028d2820100ULL,
    0x4038800480204000ULL, 0x404400140201004ULL,  0x800801001200080ULL,  0x1004900100020ULL,
    0x68200220010380cULL,  0x4100808004000200ULL, 0x4002884010610ULL,    0x801000200a04100ULL,
    0x280014004200042ULL,  0x800404008201004ULL,  0x2201090020001041ULL, 0x412808058005000ULL,
    0x3040808014008800ULL, 0x20818002000c00ULL,   0x2000040010082142ULL, 0x42802000091094cULL,
    0x80248004400bULL,     0x8204200040401000ULL, 0x12a00500410010ULL,   0x42100080080080ULL,
    0x400041100080100ULL,  0x2030820080040080ULL, 0x8012000600210408ULL, 0x1044020004c481ULL,
    0x9882400020800080ULL, 0x20082280804000ULL,   0x30e014082002090ULL,  0x400080080801000ULL,
    0x8800140080802800ULL, 0x3001004803000c00ULL, 0xc02000802000411ULL,  0x21410000890002c2ULL,
    0xc0016188c0008000ULL, 0x1010022001504000ULL, 0x813041020010040ULL,  0x850000800808032ULL,
    0xc004800808004ULL,    0x22000810020004ULL,   0xc000880102840010ULL, 0x4403344990a0014ULL,
    0x908800420400480ULL,  0x81002004c0100040ULL, 0x40a2200041001500ULL, 0x202a10010100ULL,
    0x101880204008080ULL,  0x1000800400220080ULL, 0x2101000200040100ULL, 0x66300840200ULL,
    0x4810200504a22ULL,    0x100201a824001ULL,    0x24601082000842ULL,   0x10200200c0810c2ULL,
    0x902200051018a002ULL, 0x400200080c017006ULL, 0xc00010428805020cULL, 0x4009000082044821ULL,
};

const BitBoard bishop_magic_numbers[64] = {
    0x8c431014048280ULL,   0x104e0224002023ULL,   0x4008024442000000ULL, 0x8510c0181220081ULL,
    0x90040c2000000a00ULL, 0xc416010042082ULL,    0xe21050080000ULL,     0x110a002401081802ULL,
    0x40042806080200ULL,   0x8011041c840040ULL,   0x5018300401a02300ULL, 0x110400800000ULL,
    0x424006021000004bULL, 0x2800020110884000ULL, 0xc84001820846c000ULL, 0x2108002404020882ULL,
    0x920441082100111ULL,  0x8095010009084ULL,    0x228c000800292a00ULL, 0x5000c01c04008004ULL,
    0x2800400a02220ULL,    0xc6400200501400ULL,   0x4204900100909004ULL, 0x208521500421000ULL,
    0x409140208202800ULL,  0x90c0021180a00ULL,    0x8840a0084180411ULL,  0x401404004010200ULL,
    0xa24301001410400aULL, 0x1011c082080222ULL,   0x8022109002443010ULL, 0x1003281008802ULL,
    0xa808043000400200ULL, 0x908029000284910ULL,  0x202020300348008aULL, 0x211420080c0100ULL,
    0x20040300002008ULL,   0x1102088202110800ULL, 0x100200a014cc00ULL,   0x4092004cc0061201ULL,
    0x4010882401230c6ULL,  0x800809c2000b000ULL,  0x490080414050080cULL, 0x1004402019002800ULL,
    0x1308c01028818500ULL, 0x40c0082081001020ULL, 0x10210811310080ULL,   0x1008028122000040ULL,
    0x204c208400004ULL,    0x29088c0402420000ULL, 0xa4201900000ULL,      0x800080184040038ULL,
    0x80415040020ULL,      0xd0081010828000ULL,   0x141900428808000ULL,  0x8030100a05802004ULL,
    0x94540901082000ULL,   0x288904028c9410ULL,   0x8000002602010400ULL, 0x200180218800ULL,
    0x8100020840114900ULL, 0x400604004080280ULL,  0x20242008061081ULL,   0x1810210208014100ULL,
};

extern BitBoard rook_magic_attacks[64][4096];
extern BitBoard bishop_magic_attacks[64][1024];

BitBoard generateWhitePawnPush(int index);
BitBoard generateWhitePawnDoublePush(int index);
BitBoard generateWhitePawnAttack(int index);

BitBoard generateBlackPawnPush(int index);
BitBoard generateBlackPawnDoublePush(int index);
BitBoard generateBlackPawnAttack(int index);

BitBoard generateKnightMove(int index);
BitBoard generateKingMove(int index);

BitBoard generateSlidingMove_NW(int index);
BitBoard generateSlidingMove_N(int index);
BitBoard generateSlidingMove_NE(int index);
BitBoard generateSlidingMove_E(int index);
BitBoard generateSlidingMove_SE(int index);
BitBoard generateSlidingMove_S(int index);
BitBoard generateSlidingMove_SW(int index);
BitBoard generateSlidingMove_W(int index);

void initMoves();

BitBoard generatePositiveRayAttack(int direction, int index, BitBoard occupancy);
BitBoard generateNegativeRayAttack(int direction, int index, BitBoard occupancy);

BitBoard generateBishopAttacks(int index, BitBoard occupancy);
BitBoard generateRookAttacks(int index, BitBoard occupancy);

void iterateOccupancy(BitBoard &occupancy, BitBoard mask);

void findRookMagic(int index);
void findBishopMagic(int index);

void initMagics();

static inline BitBoard getMagicBishopAttack(int index, BitBoard occupancy) {
    return bishop_magic_attacks[index][((occupancy & bishop_magic_mask[index]) *
                                        bishop_magic_numbers[index]) >>
                                       (64 - bishop_relevant_bit_count[index])];
}

static inline BitBoard getMagicRookAttack(int index, BitBoard occupancy) {
    return rook_magic_attacks[index]
                             [((occupancy & rook_magic_mask[index]) * rook_magic_numbers[index]) >>
                              (64 - rook_relevant_bit_count[index])];
}

template <Color side>
BitBoard pawnAttacksFromBitboard(BitBoard bitboard) {
    if constexpr (side == WHITE) {
        return (bitboard << 7 & NOT_H_FILE) | (bitboard << 9 & NOT_A_FILE);
    } else {
        return (bitboard >> 7 & NOT_A_FILE) | (bitboard >> 9 & NOT_H_FILE);
    }
}

BitBoard knightAttacksFromBitboard(BitBoard bitboard);
BitBoard bishopAttacksFromBitboard(BitBoard bishops, BitBoard occupancy);
BitBoard rookAttacksFromBitboard(BitBoard rooks, BitBoard occupancy);

}  // namespace Spotlight
