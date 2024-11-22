#pragma once

#include "types.hpp"
#include "utils.hpp"
#include "bitboards.hpp"

/*
Move encoding is with a 16 bit unsigned integer.

bits 0-5: start square
bits 6-11: end square
bits 12-15: move type

*/


// move type codes
const int quiet_move = 0;
const int double_pawn_push =  0b0001;
const int capture_move = 0b0100;
const int king_castle = 0b0010;
const int queen_castle = 0b0011;
const int en_passant_capture = 0b0101;
const int knight_promotion = 0b1000;
const int bishop_promotion = 0b1001;
const int rook_promotion = 0b1010;
const int queen_promotion = 0b1011;
const int knight_promotion_capture = 0b1100;
const int bishop_promotion_capture = 0b1101;
const int rook_promotion_capture = 0b1110;
const int queen_promotion_capture = 0b1111;

const int promotion_flag = 0b1000;

static inline move16 encodeMove(int start, int end, int move_type) {
    return (start | (end << 6) | (move_type << 12));
};

void printMove(move16 move);

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

        inline move16 *begin() {return &move_array[0];}
        inline move16 *end() {return &move_array[length];}

        inline size_t size() {return length;}

    private:
        move16 move_array[256];
        int length;
};

void addMovesFromBitboard(int start, U64 bb, int move_type,  MoveList &moves);
