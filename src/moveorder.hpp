#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

const int SEE_VALUES[6] = {100, 300, 300, 500, 900, 100000};
const int SEE_MULTIPLIER = 1000;

const int TT_MOVE_SCORE = (1 << 30);
const int KILLER_1_SCORE = SEE_VALUES[PAWN] / 2 * SEE_MULTIPLIER + 1;
const int KILLER_2_SCORE = KILLER_1_SCORE - 1;
const int IGNORE_MOVE = -(1 << 30);

U64 getAllAttacks(Position &pos, int sq);

int see(Position &pos, move16 move);

int scoreMove(Position &pos, move16 move);

void orderMoves(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2);


/*
Stuff to store in MovePicker for faster SEE:

* 

*/



class MovePicker {
public:
    MovePicker(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2);
    move16 getNextMove();
private:
    int move_index;
    int list_size;
    std::vector<std::pair<int, move16>> scored_moves;
};