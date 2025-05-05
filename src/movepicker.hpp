#pragma once

#include "types.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "see.hpp"

#include <memory>

namespace Spotlight {

const int HISTORY_DIVISOR = 1;
const int TT_MOVE_SCORE = (1 << 30);
const int MAX_HISTORY = 50000;
const int KILLER_1_SCORE = MAX_HISTORY * 2 + 1;
const int KILLER_2_SCORE = MAX_HISTORY * 2;
const int IGNORE_MOVE = -(1 << 30);

class MovePicker {
public:
    MovePicker(Position &_pos, int (*_quiet_history)[2][64][64], move16 _tt_move, move16 _killer_1, move16 _killer_2);
    move16 getNextMove();
    move16 getNextCapture();
    void reset();
    GenType stage;
private:
    int scoreMove(move16 move);
    move16 tt_move;
    bool tt_played;
    move16 killer_1;
    move16 killer_2;
    bool generated_captures;
    bool generated_quiets;
    MoveList captures;
    int capture_index;
    MoveList quiets;
    int quiet_index;
    Position &pos;
    int (*quiet_history)[2][64][64];

    move16 selectMove(int start, MoveList &scored_moves);
    move16 selectWinningCapture(int start, MoveList &scored_moves);
    void scoreQuiets(MoveList &moves, move16 _tt_move, move16 _killer_1, move16 _killer_2);
    void scoreCaptures(MoveList &moves, move16 _tt_move, move16 _killer_1, move16 _killer_2);
};

} // namespace Spotlight
