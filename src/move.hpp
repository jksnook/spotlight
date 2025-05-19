#pragma once

#include <string>
#include <string_view>

#include "bitboards.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace Spotlight {

/*
Move encoding is with a 16 bit unsigned integer.

bits 0-5: start square
bits 6-11: end square
bits 12-15: move type

*/

// move type codes
const move16 QUIET_MOVE = 0;
const move16 DOUBLE_PAWN_PUSH = 0b0001;
const move16 CAPTURE_MOVE = 0b0100;
const move16 KING_CASTLE = 0b0010;
const move16 QUEEN_CASTLE = 0b0011;
const move16 EN_PASSANT_CAPTURE = 0b0101;
const move16 KNIGHT_PROMOTION = 0b1000;
const move16 BISHOP_PROMOTION = 0b1001;
const move16 ROOK_PROMOTION = 0b1010;
const move16 QUEEN_PROMOTION = 0b1011;
const move16 KNIGHT_PROMOTION_CAPTURE = 0b1100;
const move16 BISHOP_PROMOTION_CAPTURE = 0b1101;
const move16 ROOK_PROMOTION_CAPTURE = 0b1110;
const move16 QUEEN_PROMOTION_CAPTURE = 0b1111;
const move16 UNUSED_MOVE_TYPE_1 = 0b0110;
const move16 UNUSED_MOVE_TYPE_2 = 0b0111;

const move16 PROMOTION_FLAG = 0b1000;

const move16 NULL_MOVE = 0;

constexpr std::string_view moveTypeToString(move16 move_type) {
    switch (move_type) {
        case QUIET_MOVE:
            return "quiet move";
        case DOUBLE_PAWN_PUSH:
            return "double pawn push";
        case CAPTURE_MOVE:
            return "capture";
        case KING_CASTLE:
            return "king castle";
        case QUEEN_CASTLE:
            return "queen castle";
        case EN_PASSANT_CAPTURE:
            return "en passant";
        case KNIGHT_PROMOTION:
            return "knight promotion";
        case BISHOP_PROMOTION:
            return "bishop promotion";
        case ROOK_PROMOTION:
            return "rook promotion";
        case QUEEN_PROMOTION:
            return "queen promotion";
        case KNIGHT_PROMOTION_CAPTURE:
            return "knight promotion capture";
        case BISHOP_PROMOTION_CAPTURE:
            return "bishop promotion capture";
        case ROOK_PROMOTION_CAPTURE:
            return "rook promotion capture";
        case QUEEN_PROMOTION_CAPTURE:
            return "queen promotion capture";
        default:
            break;
    }
    return "unknown move type";
}

static inline move16 encodeMove(Square start, Square end, move16 move_type) {
    return (start | (end << 6) | (move_type << 12));
};

static inline Square getFromSquare(const move16 &move) {
    return static_cast<Square>(move & 0b111111);
}

static inline Square getToSquare(const move16 &move) {
    return static_cast<Square>((move >> 6) & 0b111111);
}

static inline move16 getMoveType(const move16 &move) { return (move >> 12) & 0b1111; }

static inline bool isQuiet(const move16 &move) { return !((move >> 12) & 0b1111 & CAPTURE_MOVE); }

static inline bool isCaptureOrPromotion(const move16 &move) {
    return getMoveType(move) & CAPTURE_MOVE || getMoveType(move) & PROMOTION_FLAG;
}

static inline bool isCastleMove(const move16 &move_type) {
    return move_type == KING_CASTLE || move_type == QUEEN_CASTLE;
}

inline constexpr PieceType promoPiece(move16 move_type) {
    switch (move_type & ~CAPTURE_MOVE) {
        case QUEEN_PROMOTION:
            return QUEEN;
            break;
        case KNIGHT_PROMOTION:
            return KNIGHT;
            break;
        case ROOK_PROMOTION:
            return ROOK;
            break;
        case BISHOP_PROMOTION:
            return BISHOP;
            break;
        default:
            return PAWN;
            break;
    }
}

std::string moveToString(move16 move);

void printMove(move16 move);

void printMoveLong(move16 move);

struct ScoredMove {
    int score;
    move16 move;
};

class MoveList {
   public:
    MoveList() : length(0) {}

    inline void addMove(move16 move) {
        move_array[length].move = move;
        length++;
    }

    inline void setMove(int index, move16 move) { move_array[index].move = move; }

    inline void removeMove(int index) { move_array[index] = move_array[--length]; }

    inline ScoredMove &operator[](int index) { return move_array[index]; };
    inline const ScoredMove &operator[](int index) const { return move_array[index]; };

    inline ScoredMove *begin() { return &move_array[0]; }
    inline ScoredMove *end() { return &move_array[length]; }

    inline size_t size() { return length; }

   private:
    ScoredMove move_array[256];
    int length;
};

static inline void addMovesFromBitboard(Square start, BitBoard bb, move16 move_type,
                                        MoveList &moves) {
    while (bb) {
        moves.addMove(encodeMove(start, popLSB(bb), move_type));
    }
};

}  // namespace Spotlight
