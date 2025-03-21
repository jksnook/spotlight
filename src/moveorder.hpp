#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

#include <memory>

U64 getAllAttackers(Position &pos, int sq);

int see(Position &pos, move16 move);

int scoreMove(Position &pos, move16 move);

void orderMoves(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2);

