#pragma once

#include <memory>

#include "eval.hpp"
#include "move.hpp"
#include "position.hpp"

namespace Spotlight {

const int SEE_VALUES[13] = {100, 300, 300, 500, 900, 100000, 100, 300, 300, 500, 900, 100000, 0};
const int SEE_MARGIN = 50;
const int SEE_MULTIPLIER = 1000;

BitBoard getAttackersTo(Position &pos, int sq, BitBoard occupancy);

int see(Position &pos, move16 move);

bool seeGe(Position &pos, move16 move, int margin);

}  // namespace Spotlight
