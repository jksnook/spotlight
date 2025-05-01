#pragma once

#include "types.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "moveorder.hpp"
#include "moparams.hpp"

#include <memory>

namespace Spotlight {

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
    std::vector<std::pair<int, move16>> scored_captures;
    int capture_index;
    std::vector<std::pair<int, move16>> scored_quiets;
    int quiet_index;
    Position &pos;
    int (*quiet_history)[2][64][64];

    move16 selectMove(int start, std::vector<std::pair<int, move16>> &scored_moves);
    move16 selectWinningCapture(int start, std::vector<std::pair<int, move16>> &scored_moves);
    void scoreQuiets(MoveList &moves, std::vector<std::pair<int, move16>> &scored_moves, move16 _tt_move, move16 _killer_1, move16 _killer_2);
    void scoreCaptures(MoveList &moves, std::vector<std::pair<int, move16>> &scored_moves, move16 _tt_move, move16 _killer_1, move16 _killer_2);
};

} // namespace Spotlight
