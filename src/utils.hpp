#pragma once

#include <cassert>
#include <random>
#include <map>
#include <string>
#include "types.hpp"

enum { NORTHWEST, NORTH, NORTHEAST, EAST, SOUTHEAST, SOUTH, SOUTHWEST, WEST };

const int WHITE = 0;
const int BLACK = 1;
const int NUM_SQUARES = 64;

enum {WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
      BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
      WHITE_OCCUPANCY, BLACK_OCCUPANCY, OCCUPANCY, NO_PIECE};

enum {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

static std::map<char, int>  LETTER_PIECE_MAP = {
    {'P', WHITE_PAWN},
    {'N', WHITE_KNIGHT},
    {'B', WHITE_BISHOP},
    {'R', WHITE_ROOK},
    {'Q', WHITE_QUEEN},
    {'K', WHITE_KING},
    {'p', BLACK_PAWN},
    {'n', BLACK_KNIGHT},
    {'b', BLACK_BISHOP},
    {'r', BLACK_ROOK},
    {'q', BLACK_QUEEN},
    {'k', BLACK_KING}
};

static std::map<int, char>  PIECE_TO_LETTER_MAP = {
    {NO_PIECE, ' '},
    {WHITE_PAWN, 'P'},
    {WHITE_KNIGHT, 'N'},
    {WHITE_BISHOP, 'B'},
    {WHITE_ROOK, 'R'},
    {WHITE_QUEEN, 'Q'},
    {WHITE_KING, 'K'},
    {BLACK_PAWN, 'p'},
    {BLACK_KNIGHT, 'n'},
    {BLACK_BISHOP, 'b'},
    {BLACK_ROOK, 'r'},
    {BLACK_QUEEN, 'q'},
    {BLACK_KING, 'k'}
};

inline constexpr int getPieceID(int piece_type, int side) {
    if (side == WHITE) {
        return piece_type;
    } else {
        return piece_type + 6;
    }
}

inline constexpr int getOccupancy(int side) {
    if (side == BLACK) {
        return BLACK_OCCUPANCY;
    } else {
        return WHITE_OCCUPANCY;
    }
}


// castle rights codes
const int WQC = 1;
const U64 WQC_SQUARES = 0b1110ULL;
static constexpr U64 WQC_KING_SQUARES = 0b1100ULL;
const int WKC = 2;
const U64 WKC_SQUARES = 0b1100000ULL;
static constexpr U64 WKC_KING_SQUARES = 0b1100000ULL;
const int BQC = 4;
const U64 BQC_SQUARES = 0b1110ULL << (8 * 7);
static constexpr U64 BQC_KING_SQUARES = WQC_KING_SQUARES << (8 * 7);
const int BKC = 8;
const U64 BKC_SQUARES = 0b1100000ULL << (8 * 7);
static constexpr U64 BKC_KING_SQUARES = WKC_KING_SQUARES << (8 * 7);

static std::map<char, int>  CASTLE_RIGHTS_MAP = {
    {'Q', WQC},
    {'K', WKC},
    {'q', BQC},
    {'k', BKC},
    {'-', 0}
};


enum {
    a1, b1, c1, d1, e1, f1, g1, h1,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a8, b8, c8, d8, e8, f8, g8, h8,
};

enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
};

const std::string SQUARE_NAMES[64] {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", 
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8" 
};
