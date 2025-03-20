#pragma once

#include "types.hpp"
#include "utils.hpp"
#include "bitboards.hpp"

#include <map>

/*
Move encoding is with a 16 bit unsigned integer.

bits 0-5: start square
bits 6-11: end square
bits 12-15: move type

*/


// move type codes
const int QUIET_MOVE = 0;
const int DOUBLE_PAWN_PUSH =  0b0001;
const int CAPTURE_MOVE = 0b0100;
const int KING_CASTLE = 0b0010;
const int QUEEN_CASTLE = 0b0011;
const int EN_PASSANT_CAPTURE = 0b0101;
const int KNIGHT_PROMOTION = 0b1000;
const int BISHOP_PROMOTION = 0b1001;
const int ROOK_PROMOTION = 0b1010;
const int QUEEN_PROMOTION = 0b1011;
const int KNIGHT_PROMOTION_CAPTURE = 0b1100;
const int BISHOP_PROMOTION_CAPTURE = 0b1101;
const int ROOK_PROMOTION_CAPTURE = 0b1110;
const int QUEEN_PROMOTION_CAPTURE = 0b1111;
const int UNUSED_MOVE_TYPE_1 = 0b0110;
const int UNUSED_MOVE_TYPE_2 = 0b0111;

const move16 NULL_MOVE = 0;

static std::map<int, std::string>  move_type_map = {
    {QUIET_MOVE, "quiet move"},
    {DOUBLE_PAWN_PUSH, "double pawn push"},
    {CAPTURE_MOVE, "capture"},
    {KING_CASTLE, "king castle"},
    {QUEEN_CASTLE, "queen castle"},
    {EN_PASSANT_CAPTURE, "en passant"},
    {KNIGHT_PROMOTION, "knight promotion"},
    {BISHOP_PROMOTION, "bishop promotion"},
    {ROOK_PROMOTION, "rook promotion"},
    {QUEEN_PROMOTION, "queen promotion"},
    {KNIGHT_PROMOTION_CAPTURE, "knight promotion capture"},
    {BISHOP_PROMOTION_CAPTURE, "bishop promotion capture"},
    {ROOK_PROMOTION_CAPTURE, "rook promotion capture"},
    {QUEEN_PROMOTION_CAPTURE, "queen promotion capture"},
};

const int PROMOTION_FLAG = 0b1000;

static inline int getCastleSide(int move_type) {
    return move_type % 2;
};

static inline move16 encodeMove(int start, int end, int move_type) {
    return (start | (end << 6) | (move_type << 12));
};

static inline int getFromSquare(const move16 &move) {
    return move & 0b111111;
}

static inline int getToSquare(const move16 &move) {
    return (move >> 6) & 0b111111;
}

static inline int getMoveType(const move16 &move) {
    return (move >> 12) & 0b1111;
}

static inline bool isQuiet(const move16 &move) {
    return !((move >> 12) & 0b1111 & CAPTURE_MOVE);
}

std::string moveToString(move16 move);

void printMove(move16 move);

void printMoveLong(move16 move);

class MoveList {
    public:
        MoveList(): length(0) {}

        inline void addMove(unsigned long move) { 
            move_array[length] = move;
            length++;  
        }

        inline void setMove(int index, unsigned long move) {
            move_array[index] = move;
        }

        move16& operator[](int index);
        const move16& operator[](int index) const;

        inline move16 *begin() {return &move_array[0];}
        inline move16 *end() {return &move_array[length];}

        inline size_t size() {return length;}

    private:
        move16 move_array[256];
        int length;
};

static inline void addMovesFromBitboard(int start, U64 bb, int move_type,  MoveList &moves) {
    while (bb) {
        moves.addMove(encodeMove(start, popLSB(bb), move_type));
    }
};
