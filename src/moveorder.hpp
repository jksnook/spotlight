#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

U64 getAllAttacks(Position &pos, int sq);

int see(Position &pos, move16 move);

int scoreMove(Position &pos, move16 move);

void orderMoves(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2);


// MovePicker class not currently used
class MovePicker {
public:
    MovePicker(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2);
    move16 getNextMove();
private:
    int move_index;
    int list_size;
    std::vector<std::pair<int, move16>> scored_moves;
};