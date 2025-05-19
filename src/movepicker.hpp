#pragma once

#include <memory>

#include "movegen.hpp"
#include "position.hpp"
#include "see.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace Spotlight {

const int HISTORY_DIVISOR = 1;
const int TT_MOVE_SCORE = (1 << 30);
const int MAX_HISTORY = 512;
const int KILLER_1_SCORE = MAX_HISTORY * 2 + 1;
const int KILLER_2_SCORE = MAX_HISTORY * 2;
const int IGNORE_MOVE = -(1 << 30);

// TODO add stage for killer moves
enum class PickerStage { TT_MOVE, GOOD_NOISY, KILLER_1, KILLER_2, QUIET_AND_BAD_NOISY, END };

class MovePicker {
   public:
    MovePicker(Position &_pos, int (*_quiet_history)[2][64][64], move16 _tt_move, move16 _killer_1,
               move16 _killer_2);
    move16 getNextMove();
    move16 getNextCapture();
    PickerStage stage;

   private:
    int scoreNoisyMove(move16 move);
    int scoreQuietMove(move16 move);
    move16 tt_move;
    bool tt_played;
    move16 killer_1;
    move16 killer_2;
    MoveList noisy_moves;
    uint16_t capture_index;
    MoveList quiets;
    uint16_t quiet_index;
    bool generated_noisies;
    bool generated_quiets;
    Position &pos;
    int (*quiet_history)[2][64][64];

    move16 selectMove(int start, MoveList &scored_moves);
    move16 selectWinningCapture(int start, MoveList &scored_moves);
    void scoreQuiets();
    void scoreNoisies();
};

}  // namespace Spotlight
