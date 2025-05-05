#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

#include <memory>

namespace Spotlight {

const int SEE_VALUES[6] = {100, 300, 300, 500, 900, 100000};
const int SEE_MARGIN = 50;
const int SEE_MULTIPLIER = 1000;

BitBoard getAllAttackers(Position &pos, int sq);

int see(Position &pos, move16 move);

} // namespace Spotlight
