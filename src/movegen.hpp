#pragma once

#include "types.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "bitboards.hpp"
#include "move.hpp"

#include <iostream>

namespace Spotlight {

U64 getEnemyAttacks(Position &pos, Square sq);

template <Color side>
BitBoard getCheckers(Position& pos, Square king_index);

template <Color side>
U64 getAllEnemyAttacks(Position& pos);

template <Color side, GenType gen_type>
void generateMovesSided(MoveList& moves, Position& pos);

void generateCaptures(MoveList &moves, Position &pos);

void generateQuietMoves(MoveList &moves, Position &pos);

void generateMoves(MoveList &moves, Position &pos);

template <Color side>
bool inCheckSided(Position& pos);

bool inCheck(Position &pos);

bool otherSideInCheck(Position& pos);

bool isPseudoLegal(move16 move, Position &pos);

bool isLegal(move16 move, Position &pos);

U64 perftHelper(Position &pos, int depth);

U64 perft(Position &pos, int depth);

} // namespace Spotlight
