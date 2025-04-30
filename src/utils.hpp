#pragma once

#include "types.hpp"

#include <string_view>

namespace Spotlight {

const uint8_t WHITE_OCCUPANCY = 12;
const uint8_t BLACK_OCCUPANCY = 13;
const uint8_t OCCUPANCY = 14;

constexpr std::string_view PIECE_TO_LETTER_MAP = "PNBRQKpnbrqk    ";

constexpr Piece letterToPiece(char c) {
    for (int i = 0; i <= static_cast<int>(Piece::BLACK_KING); i++) {
        if (PIECE_TO_LETTER_MAP[i] == c) {
            return static_cast<Piece>(i);
        }
    }
    return Piece::NO_PIECE;
}

inline constexpr Color getOtherSide(Color side) {
    return static_cast<Color>(static_cast<int>(side) ^ 1);
}

inline constexpr Piece getPieceID(PieceType piece_type, Color side) {
    return static_cast<Piece>( static_cast<int>(piece_type) + static_cast<int>(side) * 6 );
}

inline constexpr Color getPieceColor(Piece piece) {
    return static_cast<Color>(piece > Piece::WHITE_KING);
}

inline constexpr int getOccupancy(Color side) {
    if (side == Color::BLACK) {
        return BLACK_OCCUPANCY;
    } else {
        return WHITE_OCCUPANCY;
    }
}

// castle rights codes
const int WQC = 1;
const int WKC = 2;
const int BQC = 4;
const int BKC = 8;

// castle squares needed for move generation and validation
const BitBoard WQC_SQUARES = 0b1110ULL;
const BitBoard WQC_KING_SQUARES = 0b1100ULL;
const BitBoard WKC_SQUARES = 0b1100000ULL;
const BitBoard WKC_KING_SQUARES = 0b1100000ULL;
const BitBoard BQC_SQUARES = 0b1110ULL << (8 * 7);
const BitBoard BQC_KING_SQUARES = WQC_KING_SQUARES << (8 * 7);
const BitBoard BKC_SQUARES = 0b1100000ULL << (8 * 7);
const BitBoard BKC_KING_SQUARES = WKC_KING_SQUARES << (8 * 7);

constexpr std::string_view SQUARE_NAMES[64] {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1", 
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2", 
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", 
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4", 
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5", 
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", 
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7", 
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8" 
};

constexpr std::string_view STARTPOS = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

}