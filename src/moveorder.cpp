#include "moveorder.hpp"

int scoreMove(Position &pos, move16 move) {
    int move_type = getMoveType(move);
    switch (move_type)
    {
    case capture_move:
        return piece_values[pos.at(getToSquare(move)) % 6] - piece_values[pos.at(getFromSquare(move)) % 6] + 500;
        break;
    default:
        return 0;
        break;
    }
}

void orderMoves(Position &pos, MoveList &moves, move16 tt_move) {
    int s = moves.size();
    std::vector<std::pair<int, move16>> sorted_moves(s);

    for (int i = 0; i < s; i++) {
        if (moves[i] == tt_move) {
            sorted_moves[i] = {2000, moves[i]};
        } else {
            sorted_moves[i] = {scoreMove(pos, moves[i]), moves[i]};
        }
    }

    std::sort(sorted_moves.begin(), sorted_moves.end());

    for (int i = 0; i < s; i++) {
        moves[s - i - 1] = sorted_moves[i].second;
    }
}
