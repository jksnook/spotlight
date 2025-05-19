#include "move.hpp"

#include <iostream>

namespace Spotlight {

std::string moveToString(move16 move) {
    std::string s = "";

    Square start = getFromSquare(move);
    Square end = getToSquare(move);
    int move_type = getMoveType(move);
    s = static_cast<std::string>(SQUARE_NAMES[start]) + static_cast<std::string>(SQUARE_NAMES[end]);
    if (PROMOTION_FLAG & move_type) {
        move_type &= QUEEN_PROMOTION;
        if (move_type == QUEEN_PROMOTION) {
            s += 'q';
        } else if (move_type == KNIGHT_PROMOTION) {
            s += 'n';
        } else if (move_type == BISHOP_PROMOTION) {
            s += 'b';
        } else if (move_type == ROOK_PROMOTION) {
            s += 'r';
        }
    }

    return s;
}

void printMove(move16 move) { std::cout << moveToString(move) << "\n"; };

void printMoveLong(move16 move) {
    int move_type = getMoveType(move);

    std::cout << moveToString(move) << " " << moveTypeToString(move_type) << "\n";
};

}  // namespace Spotlight