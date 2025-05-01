#pragma once

namespace Spotlight {

const int SEE_VALUES[6] = {100, 300, 300, 500, 900, 100000};
const int SEE_MARGIN = 50;
const int SEE_MULTIPLIER = 1000;
const int HISTORY_DIVISOR = 1;

const int TT_MOVE_SCORE = (1 << 30);
const int KILLER_1_SCORE = SEE_VALUES[PAWN] / 2 * SEE_MULTIPLIER + 1;
const int KILLER_2_SCORE = KILLER_1_SCORE - 1;
const int IGNORE_MOVE = -(1 << 30);
const int MAX_HISTORY = (KILLER_2_SCORE - 10) * HISTORY_DIVISOR;

} // namespace Spotlight
