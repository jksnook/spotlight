#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

const int see_values[6] = {100, 300, 300, 500, 900, 100000};

const int TT_MOVE_SCORE = (1 << 30);
const int IGNORE_MOVE = -(1 << 30);

U64 getAllAttacks(Position &pos, int sq);

int see(Position &pos, move16 move);

int scoreMove(Position &pos, move16 move);

void orderMoves(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2);