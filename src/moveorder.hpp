#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

#include <vector>
#include <algorithm>

int scoreMove(Position &pos, move16 move);

void orderMoves(Position &pos, MoveList &moves, move16 tt_move);