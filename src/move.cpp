#include "move.hpp"

#include <iostream>

void printMove(move16 move) {
    int start = (move) & 0b111111;
    int end = (move >> 6) & 0b111111;
    int move_type = (move >> 12) & 0b1111;
    std::cout << square_names[start] << square_names[end];
    if (0b1000 & move_type) {
        if (move_type == queen_promotion_capture) {
            std::cout << 'q';
        } else if (move_type == knight_promotion_capture) {
            std::cout << 'n';
        } else if (move_type == bishop_promotion_capture) {
            std::cout << 'b';
        } else if (move_type == rook_promotion_capture) {
            std::cout << 'r';
        }
    }

    std::cout << "\n";
};

void addMovesFromBitboard(int start, U64 bb, int move_type,  MoveList &moves) {
    while (bb) {
        moves.addMove(encodeMove(start, popLSB(bb), move_type));
    }
};