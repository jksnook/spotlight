#include "move.hpp"

#include <iostream>

move16& MoveList::operator[](int index){
    return move_array[index];
};

const move16& MoveList::operator[](int index) const {
    return move_array[index];
};

std::string moveToString(move16 move) {
    std::string s = "";

    int start = getFromSquare(move);
    int end = getToSquare(move);
    int move_type = getMoveType(move);
    s = SQUARE_NAMES[start] + SQUARE_NAMES[end];
    if (PROMOTION_FLAG & move_type) {
        move_type &= QUEEN_PROMOTION;
        if (move_type == QUEEN_PROMOTION) {
            s+= 'q';
        } else if (move_type == KNIGHT_PROMOTION) {
            s+= 'n';
        } else if (move_type == BISHOP_PROMOTION) {
            s+= 'b';
        } else if (move_type == ROOK_PROMOTION) {
            s+= 'r';
        }
    }

    return s;
}

void printMove(move16 move) {
    std::cout << moveToString(move) << "\n";
};

void printMoveLong(move16 move) {
    int move_type = getMoveType(move);

    std::cout << moveToString(move) << " " << move_type_map[move_type] << "\n";
};
