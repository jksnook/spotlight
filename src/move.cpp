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
    s = square_names[start] + square_names[end];
    if (promotion_flag & move_type) {
        move_type &= queen_promotion;
        if (move_type == queen_promotion) {
            s+= 'q';
        } else if (move_type == knight_promotion) {
            s+= 'n';
        } else if (move_type == bishop_promotion) {
            s+= 'b';
        } else if (move_type == rook_promotion) {
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
