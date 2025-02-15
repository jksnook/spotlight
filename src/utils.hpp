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

enum {white_pawn, white_knight, white_bishop, white_rook, white_queen, white_king,
      black_pawn, black_knight, black_bishop, black_rook, black_queen, black_king,
      white_occupancy, black_occupancy, occupancy};

enum {WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
      BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
      WHITE_OCCUPANCY, BLACK_OCCUPANCY, OCCUPANCY, NO_PIECE};

enum {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};

static std::map<char, int>  letter_piece_map = {
    {'P', white_pawn},
    {'N', white_knight},
    {'B', white_bishop},
    {'R', white_rook},
    {'Q', white_queen},
    {'K', white_king},
    {'p', black_pawn},
    {'n', black_knight},
    {'b', black_bishop},
    {'r', black_rook},
    {'q', black_queen},
    {'k', black_king}
};

static std::map<int, char>  piece_to_letter_map = {
    {NO_PIECE, ' '},
    {white_pawn, 'P'},
    {white_knight, 'N'},
    {white_bishop, 'B'},
    {white_rook, 'R'},
    {white_queen, 'Q'},
    {white_king, 'K'},
    {black_pawn, 'p'},
    {black_knight, 'n'},
    {black_bishop, 'b'},
    {black_rook, 'r'},
    {black_queen, 'q'},
    {black_king, 'k'}
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
        return black_occupancy;
    } else {
        return white_occupancy;
    }
}

const int wqc = 1;
const U64 wqc_squares = 0b1110ULL;
static constexpr U64 wqc_king_squares = 0b1100ULL;
const int wkc = 2;
const U64 wkc_squares = 0b1100000ULL;
static constexpr U64 wkc_king_squares = 0b1100000ULL;
const int bqc = 4;
const U64 bqc_squares = 0b1110ULL << (8 * 7);
static constexpr U64 bqc_king_squares = wqc_king_squares << (8 * 7);
const int bkc = 8;
const U64 bkc_squares = 0b1100000ULL << (8 * 7);
static constexpr U64 bkc_king_squares = wkc_king_squares << (8 * 7);

static std::map<char, int>  castle_rights_map = {
    {'Q', wqc},
    {'K', wkc},
    {'q', bqc},
    {'k', bkc},
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

const std::string square_names[64] {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", 
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8" 
};
