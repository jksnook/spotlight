#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

#include <vector>
#include <algorithm>

const int see_values[6] = {100, 300, 300, 500, 900, 100000};

U64 getAllAttacks(Position &pos, int sq);

int see(Position &pos, move16 move);

int scoreMove(Position &pos, move16 move);

void orderMoves(Position &pos, MoveList &moves, move16 tt_move);