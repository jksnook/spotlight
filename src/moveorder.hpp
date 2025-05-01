#pragma once

#include "move.hpp"
#include "position.hpp"
#include "eval.hpp"

#include <memory>

namespace Spotlight {

U64 getAllAttackers(Position &pos, int sq);

int see(Position &pos, move16 move);

} // namespace Spotlight
