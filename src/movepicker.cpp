#include "movepicker.hpp"

#include <algorithm>

namespace Spotlight {

MovePicker::MovePicker(Position &_pos, int (*_quiet_history)[2][64][64], move16 _tt_move, move16 _killer_1, move16 _killer_2): 
stage(TT_MOVE), tt_move(_tt_move), killer_1(_killer_1), killer_2(_killer_2), capture_index(0), quiet_index(0),
generated_captures(false), generated_quiets(false), tt_played(false), pos(_pos), quiet_history(_quiet_history) {}

move16 MovePicker::selectMove(int start, MoveList &scored_moves) {
    int best_score = IGNORE_MOVE - 1;
    int k;
    int s = scored_moves.size();
    for (int i = start; i < s; i++) {
        if (scored_moves[i].score > best_score) {
            k = i;
            best_score = scored_moves[i].score;
        }
    }
    move16 temp = scored_moves[k].move;
    scored_moves[k] = scored_moves[start];
    //scored_moves[start] = temp;
    return temp;
}

move16 MovePicker::selectWinningCapture(int start, MoveList &scored_moves) {
    int best_score = IGNORE_MOVE - 1;
    int k;
    int s = scored_moves.size();
    for (int i = start; i < s; i++) {
        if (scored_moves[i].score > best_score) {
            k = i;
            best_score = scored_moves[i].score;
        }
    }
    if (best_score < 0) {
        return NULL_MOVE;
    }
    move16 temp = scored_moves[k].move;
    scored_moves[k] = scored_moves[start];
    //scored_moves[start] = temp;
    return temp;
}

int MovePicker::scoreMove(move16 move) {
    int move_type = getMoveType(move);
    int piece;
    int from;
    switch (move_type)
    {
    case CAPTURE_MOVE:
        return see(pos, move);
        break;
    case QUIET_MOVE:
        return (*quiet_history)[pos.side_to_move][getFromSquare(move)][getToSquare(move)] / HISTORY_DIVISOR;
    case DOUBLE_PAWN_PUSH:
        return (*quiet_history)[pos.side_to_move][getFromSquare(move)][getToSquare(move)] / HISTORY_DIVISOR + 1;
        break;
    case QUEEN_PROMOTION:
        return (SEE_VALUES[QUEEN] - SEE_VALUES[PAWN]) * SEE_MULTIPLIER + 
            (*quiet_history)[pos.side_to_move][getFromSquare(move)][getToSquare(move)] / HISTORY_DIVISOR;
        break;
    case ROOK_PROMOTION:
        return IGNORE_MOVE;
        break;
    case BISHOP_PROMOTION:
        return IGNORE_MOVE;
        break;
    case KNIGHT_PROMOTION:
        return 0;
        break;
    case QUEEN_PROMOTION_CAPTURE:
        return see(pos, move);
        break;
    case ROOK_PROMOTION_CAPTURE:
        return IGNORE_MOVE;
        break;
    case BISHOP_PROMOTION_CAPTURE:
        return IGNORE_MOVE;
        break;
    case KNIGHT_PROMOTION_CAPTURE:
        return see(pos, move);
        break;
    case EN_PASSANT_CAPTURE:
        return see(pos, move);
        break;
    default:
        return 0;
        break;
    }
}

void MovePicker::scoreQuiets(MoveList &moves, move16 _tt_move, move16 _killer_1, move16 _killer_2) {
    int s = moves.size();

    for (int i = 0; i < s; i++) {
        if (moves[i].move == _tt_move) {
            moves.removeMove(i);
            i--;
            s--;
            continue;
        } else if (moves[i].move == _killer_1) {
            moves[i].score = KILLER_1_SCORE;
        } else if (moves[i].move == _killer_2) {
            moves[i].score = KILLER_2_SCORE;
        } else {
            moves[i].score = scoreMove(moves[i].move);
        }
    }

}
void MovePicker::scoreCaptures(MoveList &moves, move16 _tt_move, move16 _killer_1, move16 _killer_2) {
    int s = moves.size();

    for (int i = 0; i < s; i++) {
        if (moves[i].move == _tt_move) {
            moves.removeMove(i);
            i--;
            s--;
            continue;
        } else {
            moves[i].score = scoreMove(moves[i].move);
        }
    }
}

move16 MovePicker::getNextMove() {
    if (stage == TT_MOVE || !tt_played) {
        if (stage == TT_MOVE) {
            stage = CAPTURES_AND_PROMOTIONS;
        }
        tt_played = true;
        if (tt_move && isLegal(tt_move, pos)) {
            return tt_move;
        }

        // debug so I know if I get a bad tt move
        // assert(!tt_move);
    }

    if (stage == CAPTURES_AND_PROMOTIONS) {
        if (!generated_captures) {
            generateCaptures(captures, pos);
            generated_captures = true;

            scoreCaptures(captures, tt_move, killer_1, killer_2);
        }
        if (capture_index < captures.size()) {
            move16 m = selectWinningCapture(capture_index, captures);
            if (m) {
                capture_index++;
                return m;
            } else {
                stage = QUIET;
            }
        } else {
            stage = QUIET;
        }
    }

    if (stage == QUIET) {
        if (!generated_quiets) {
            generateQuietMoves(quiets, pos);
            generated_quiets = true;

            scoreQuiets(quiets, tt_move, killer_1, killer_2);
        }
        if (quiet_index < quiets.size()) {
            move16 m = selectMove(quiet_index, quiets);
            quiet_index++;
            return m;
        } else if (capture_index < captures.size()) {
            move16 m = selectMove(capture_index, captures);
            capture_index++;
            return m;
        } else {
            stage == END_MOVEGEN;
        }
    }

    return 0;
}

move16 MovePicker::getNextCapture() {
    if (stage == TT_MOVE) {
        stage = CAPTURES_AND_PROMOTIONS;
        if (!tt_move) {
            tt_played = true;
        } else if ((getMoveType(tt_move) & CAPTURE_MOVE || getMoveType(tt_move) & PROMOTION_FLAG) && isLegal(tt_move, pos)) {
            tt_played = true;
            return tt_move;
        }

        // debug so I know if I get a bad tt move
        // assert(!(tt_move && (getMoveType(tt_move) & CAPTURE_MOVE || getMoveType(tt_move) & PROMOTION_FLAG)));
    }

    if (stage == CAPTURES_AND_PROMOTIONS) {
        if (!generated_captures) {
            generateCaptures(captures, pos);
            generated_captures = true;

            scoreCaptures(captures, tt_move, killer_1, killer_2);
        }
        if (capture_index < captures.size()) {
            move16 m = selectMove(capture_index, captures);
            capture_index++;
            return m;
        } else {
            stage = QUIET;
        }
    }

    return 0;
}

void MovePicker::reset() {
    stage = TT_MOVE;
    tt_played = false;
    capture_index = 0;
    quiet_index = 0;
}

} // namespace Spotligh 
