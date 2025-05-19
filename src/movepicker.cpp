#include "movepicker.hpp"

#include <algorithm>

namespace Spotlight {

MovePicker::MovePicker(Position &_pos, int (*_quiet_history)[2][64][64], move16 _tt_move,
                       move16 _killer_1, move16 _killer_2)
    : stage(PickerStage::TT_MOVE),
      tt_move(_tt_move),
      tt_played(false),
      killer_1(_killer_1),
      killer_2(_killer_2),
      capture_index(0),
      quiet_index(0),
      generated_noisies(false),
      generated_quiets(false),
      pos(_pos),
      quiet_history(_quiet_history) {}

// Selection sort for getting the highest scored move
move16 MovePicker::selectMove(int start, MoveList &scored_moves) {
    int best_score = IGNORE_MOVE - 1;
    int k = 0;
    int s = scored_moves.size();
    for (int i = start; i < s; i++) {
        if (scored_moves[i].score > best_score) {
            k = i;
            best_score = scored_moves[i].score;
        }
    }
    move16 temp = scored_moves[k].move;
    scored_moves[k] = scored_moves[start];
    // scored_moves[start] = temp;
    return temp;
}

// Select a move with score > 0, otherwise return null move
move16 MovePicker::selectWinningCapture(int start, MoveList &scored_moves) {
    int best_score = IGNORE_MOVE - 1;
    int k = 0;
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
    // scored_moves[start] = temp;
    return temp;
}

int MovePicker::scoreNoisyMove(move16 move) {
    move16 move_type = getMoveType(move);
    switch (move_type) {
        case CAPTURE_MOVE:
            // currently using the SEE value alone for scoring captures. I think SEE to determine
            // good/bad plus MVV/LVA to determine ranking is a bit more standard. It didn't make
            // much difference for me.
            return see(pos, move);
            break;
        case QUEEN_PROMOTION:
            return (SEE_VALUES[QUEEN] - SEE_VALUES[PAWN]) * SEE_MULTIPLIER;
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

int MovePicker::scoreQuietMove(move16 move) {
    move16 move_type = getMoveType(move);
    Square from = getFromSquare(move);
    Square to = getToSquare(move);

    int score = (*quiet_history)[pos.side_to_move][from][to] / HISTORY_DIVISOR;

    if (move_type == DOUBLE_PAWN_PUSH) score++;

    return score;
}

void MovePicker::scoreQuiets() {
    int s = quiets.size();

    for (int i = 0; i < s; i++) {
        // quiets[i].score = scoreMove(quiets[i].move);
        if (quiets[i].move == tt_move || quiets[i].move == killer_1 || quiets[i].move == killer_2) {
            quiets.removeMove(i);
            i--;
            s--;
            continue;
        } else {
            quiets[i].score = scoreQuietMove(quiets[i].move);
        }
    }
}

void MovePicker::scoreNoisies() {
    int s = noisy_moves.size();

    for (int i = 0; i < s; i++) {
        // noisy_moves[i].score = scoreMove(noisy_moves[i].move);
        if (noisy_moves[i].move == tt_move) {
            noisy_moves.removeMove(i);
            i--;
            s--;
            continue;
        } else if (noisy_moves[i].move == killer_1) {
            killer_1 = NULL_MOVE;
            noisy_moves[i].score = scoreNoisyMove(noisy_moves[i].move);
        } else if (noisy_moves[i].move == killer_2) {
            killer_2 = NULL_MOVE;
            noisy_moves[i].score = scoreNoisyMove(noisy_moves[i].move);
        } else {
            noisy_moves[i].score = scoreNoisyMove(noisy_moves[i].move);
        }
    }
}

move16 MovePicker::getNextMove() {
    if (stage == PickerStage::TT_MOVE || !tt_played) {
        if (stage == PickerStage::TT_MOVE) {
            stage = PickerStage::GOOD_NOISY;
        }
        tt_played = true;
        if (tt_move && isLegal(tt_move, pos)) {
            return tt_move;
        }

        // debug so I know if I get a bad tt move
        // assert(!tt_move);
    }

    if (stage == PickerStage::GOOD_NOISY) {
        if (!generated_noisies) {
            generateNoisyMoves(noisy_moves, pos);
            generated_noisies = true;

            scoreNoisies();
        }
        if (capture_index < noisy_moves.size()) {
            move16 m = selectWinningCapture(capture_index, noisy_moves);
            if (m) {
                capture_index++;
                return m;
            } else {
                stage = PickerStage::KILLER_1;
            }
        } else {
            stage = PickerStage::KILLER_1;
        }
    }

    if (stage == PickerStage::KILLER_1) {
        stage = PickerStage::KILLER_2;
        if (killer_1 && isLegal(killer_1, pos)) return killer_1;
    }

    if (stage == PickerStage::KILLER_2) {
        stage = PickerStage::QUIET_AND_BAD_NOISY;
        if (killer_2 && isLegal(killer_2, pos)) return killer_2;
    }

    if (stage == PickerStage::QUIET_AND_BAD_NOISY) {
        if (!generated_quiets) {
            generateQuietMoves(quiets, pos);
            generated_quiets = true;

            scoreQuiets();
        }
        if (quiet_index < quiets.size()) {
            move16 m = selectMove(quiet_index, quiets);
            quiet_index++;
            return m;
        } else if (capture_index < noisy_moves.size()) {
            move16 m = selectMove(capture_index, noisy_moves);
            capture_index++;
            return m;
        } else {
            stage = PickerStage::END;
        }
    }

    return NULL_MOVE;
}

move16 MovePicker::getNextCapture() {
    if (stage == PickerStage::TT_MOVE) {
        stage = PickerStage::GOOD_NOISY;
        if (!tt_move) {
            tt_played = true;
        } else if (isCaptureOrPromotion(tt_move) && isLegal(tt_move, pos)) {
            tt_played = true;
            return tt_move;
        }

        // debug so I know if I get a bad tt move
        // assert(!(tt_move && (getMoveType(tt_move) & CAPTURE_MOVE || getMoveType(tt_move) &
        // PROMOTION_FLAG)));
    }

    if (stage == PickerStage::GOOD_NOISY) {
        if (!generated_noisies) {
            generateNoisyMoves(noisy_moves, pos);
            generated_noisies = true;

            scoreNoisies();
        }
        if (capture_index < noisy_moves.size()) {
            move16 m = selectMove(capture_index, noisy_moves);
            capture_index++;
            return m;
        } else {
            stage = PickerStage::KILLER_1;
        }
    }

    return NULL_MOVE;
}

}  // namespace Spotlight
