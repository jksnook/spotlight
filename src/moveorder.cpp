#include "moveorder.hpp"
#include "tunables.hpp"

#include <vector>
#include <algorithm>

U64 getAllAttacks(Position &pos, int sq) {
    U64 attackers = 0ULL;

    attackers |= knight_moves[sq] & (pos.bitboards[WHITE_KNIGHT] | pos.bitboards[BLACK_KNIGHT]);
    attackers |= pawn_attacks[BLACK][sq] & pos.bitboards[WHITE_PAWN];
    attackers |= pawn_attacks[WHITE][sq] & pos.bitboards[BLACK_PAWN];
    U64 bishop_rays_from_sq = getMagicBishopAttack(sq, pos.bitboards[OCCUPANCY]);
    attackers |= bishop_rays_from_sq & (pos.bitboards[WHITE_BISHOP] | pos.bitboards[WHITE_QUEEN]);
    attackers |= bishop_rays_from_sq & (pos.bitboards[BLACK_BISHOP] | pos.bitboards[BLACK_QUEEN]);
    U64 rook_rays_from_sq = getMagicRookAttack(sq, pos.bitboards[OCCUPANCY]);
    attackers |= rook_rays_from_sq & (pos.bitboards[WHITE_ROOK] | pos.bitboards[WHITE_QUEEN]);
    attackers |= rook_rays_from_sq & (pos.bitboards[BLACK_ROOK] | pos.bitboards[BLACK_QUEEN]);
    attackers |= king_moves[sq] & (pos.bitboards[WHITE_KING] | pos.bitboards[BLACK_KING]);

    return attackers;
}

void refreshXraysDiagonal(Position &pos, int sq, U64 remaining_occupancy, U64 &attackers_bb) {
    U64 bishop_rays_from_to_sq = getMagicBishopAttack(sq, remaining_occupancy);
    attackers_bb |= bishop_rays_from_to_sq & (pos.bitboards[BLACK_BISHOP] | pos.bitboards[BLACK_QUEEN]);
    attackers_bb |= bishop_rays_from_to_sq & (pos.bitboards[WHITE_BISHOP] | pos.bitboards[WHITE_QUEEN]);
    attackers_bb &= remaining_occupancy;
}

void refreshXraysRanksFiles(Position &pos, int sq, U64 remaining_occupancy, U64 &attackers_bb) {
    U64 rook_rays_from_to_sq = getMagicRookAttack(sq, remaining_occupancy);
    attackers_bb |= rook_rays_from_to_sq & (pos.bitboards[BLACK_ROOK] | pos.bitboards[BLACK_QUEEN]);
    attackers_bb |= rook_rays_from_to_sq & (pos.bitboards[WHITE_ROOK] | pos.bitboards[WHITE_QUEEN]);
    attackers_bb &= remaining_occupancy;
}

// Static exchange evaluation for move ordering
int see(Position &pos, move16 move) {
    int side = pos.side_to_move ^ 1;
    int to_sq = getToSquare(move);
    int from_sq = getFromSquare(move);
    int move_type = getMoveType(move);
    U64 attackers_bb = getAllAttacks(pos, to_sq);
    U64 remaining_occupancy = pos.bitboards[OCCUPANCY];
    int capture_scores[32];
    int attacking_piece;
    if (move_type == EN_PASSANT_CAPTURE) {
        attacking_piece = BLACK_PAWN - side * BLACK_PAWN;
        capture_scores[0] = SEE_VALUES[PAWN];
    } else {
        attacking_piece = pos.at(from_sq);
        capture_scores[0] = SEE_VALUES[pos.at(to_sq) % 6];
    }
    assert(attacking_piece != NO_PIECE);
    
    // if this is a promotion consider the additional change in material
    if (move_type & PROMOTION_FLAG) {
        switch (move_type)
        {
        case QUEEN_PROMOTION_CAPTURE:
            capture_scores[0] += SEE_VALUES[QUEEN] - SEE_VALUES[PAWN];
            attacking_piece = WHITE_QUEEN + BLACK_PAWN * side;
            break;
        case KNIGHT_PROMOTION_CAPTURE:
            capture_scores[0] += SEE_VALUES[KNIGHT] - SEE_VALUES[PAWN];
            attacking_piece = WHITE_KNIGHT + BLACK_PAWN * side;
            break;
        case BISHOP_PROMOTION_CAPTURE:
            capture_scores[0] += SEE_VALUES[BISHOP] - SEE_VALUES[PAWN];
            attacking_piece = WHITE_BISHOP + BLACK_PAWN * side;
            break;
        case ROOK_PROMOTION_CAPTURE:
            capture_scores[0] += SEE_VALUES[ROOK] - SEE_VALUES[PAWN];
            attacking_piece = WHITE_ROOK + BLACK_PAWN * side;
            break;
        default:
            break;
        }
    }
    int attacking_piece_type = attacking_piece % 6;


    // simulate the initial capture
    U64 current_attacker_bb = 1ULL << from_sq;
    attackers_bb &= ~current_attacker_bb;
    remaining_occupancy &= ~current_attacker_bb;
    U64 diagonal_xray_pieces = pos.bitboards[WHITE_PAWN] | pos.bitboards[BLACK_PAWN] | pos.bitboards[WHITE_BISHOP] | 
        pos.bitboards[BLACK_BISHOP] | pos.bitboards[WHITE_QUEEN] | pos.bitboards[BLACK_QUEEN];
    U64 vertical_xray_pieces = pos.bitboards[WHITE_ROOK] | pos.bitboards[BLACK_ROOK] | pos.bitboards[WHITE_QUEEN] | 
        pos.bitboards[BLACK_QUEEN];

    capture_scores[1] = SEE_VALUES[attacking_piece % BLACK_PAWN] - capture_scores[0];

    // extra updates for en passant
    if (move_type == EN_PASSANT_CAPTURE) {
        remaining_occupancy &= ~(1ULL << (to_sq - 8 * side + 8 * (1 - side)));
        refreshXraysRanksFiles(pos, to_sq, remaining_occupancy, attackers_bb);
    }

    if (current_attacker_bb & diagonal_xray_pieces) refreshXraysDiagonal(pos, to_sq, remaining_occupancy, attackers_bb);
    if (current_attacker_bb & vertical_xray_pieces) refreshXraysRanksFiles(pos, to_sq, remaining_occupancy, attackers_bb);

    // simulate the rest of the captures
    int k = 2;
    while(true) {
        current_attacker_bb = 0ULL;
        // loop through bitboards to find the least valuable attacker
        for (int p = WHITE_PAWN + BLACK_PAWN * side; p <= WHITE_KING + BLACK_PAWN * side; p++) {
            if (pos.bitboards[p] & attackers_bb) {
                attacking_piece = p;
                current_attacker_bb = (pos.bitboards[p] & attackers_bb) & -(pos.bitboards[p] & attackers_bb);
                break;
            }
        }
        if (!current_attacker_bb) {
            break;
        }

        capture_scores[k] = SEE_VALUES[attacking_piece % BLACK_PAWN] - capture_scores[k - 1];
        remaining_occupancy &= ~current_attacker_bb;

        if (current_attacker_bb & diagonal_xray_pieces) {
            refreshXraysDiagonal(pos, to_sq, remaining_occupancy, attackers_bb);
        }
        if (current_attacker_bb & vertical_xray_pieces) {
            refreshXraysRanksFiles(pos, to_sq, remaining_occupancy, attackers_bb);
        }

        k++;
        side ^= 1;
        attackers_bb ^= current_attacker_bb;
    }

    for (k -= 2;k > 0; k--) {
        capture_scores[k - 1] = -std::max(-capture_scores[k - 1], capture_scores[k]);
    }

    return capture_scores[0] * SEE_MULTIPLIER;
}

int scoreMove(Position &pos, move16 move) {
    int move_type = getMoveType(move);
    int piece;
    int from;
    switch (move_type)
    {
    case CAPTURE_MOVE:
        return see(pos, move);
        break;
    case QUIET_MOVE:
        return pos.history_table[pos.side_to_move][getFromSquare(move)][getToSquare(move)] / HISTORY_DIVISOR;
    case DOUBLE_PAWN_PUSH:
        return pos.history_table[pos.side_to_move][getFromSquare(move)][getToSquare(move)] / HISTORY_DIVISOR;
        break;
    case QUEEN_PROMOTION:
        return (SEE_VALUES[QUEEN] - SEE_VALUES[PAWN]) * SEE_MULTIPLIER + 
            pos.history_table[pos.side_to_move][getFromSquare(move)][getToSquare(move)] / HISTORY_DIVISOR;
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

void orderMoves(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2) {
    int s = moves.size();
    std::vector<std::pair<int, move16>> sorted_moves(s);

    for (int i = 0; i < s; i++) {
        if (moves[i] == tt_move) {
            sorted_moves[i] = {TT_MOVE_SCORE, moves[i]};
        } else if (moves[i] == killer_1) {
            sorted_moves[i] = {KILLER_1_SCORE, moves[i]};
        } else if (moves[i] == killer_2) {
            sorted_moves[i] = {KILLER_2_SCORE, moves[i]};
        } else {
            sorted_moves[i] = {scoreMove(pos, moves[i]), moves[i]};
        }
    }

    std::sort(sorted_moves.begin(), sorted_moves.end());

    for (int i = 0; i < s; i++) {
        moves[s - i - 1] = sorted_moves[i].second;
    }
}

MovePicker::MovePicker(Position &pos, MoveList &moves, move16 tt_move, move16 killer_1, move16 killer_2): move_index(0) {
    scored_moves.resize(moves.size());
    int s = moves.size();
    list_size = s;
    for (int i = 0; i < s; i++) {
        if (moves[i] == tt_move) {
            scored_moves[i] = {TT_MOVE_SCORE, moves[i]};
        } else if (moves[i] == killer_1) {
            scored_moves[i] = {KILLER_1_SCORE, moves[i]};
        } else if (moves[i] == killer_2) {
            scored_moves[i] = {KILLER_2_SCORE, moves[i]};
        } else {
            scored_moves[i] = {scoreMove(pos, moves[i]), moves[i]};
        }
    }
}

move16 MovePicker::getNextMove() {
    if (move_index >= list_size) {
        return 0;
    }
    int best_score = IGNORE_MOVE - 1;
    int k;
    for (int i = move_index; i < list_size; i++) {
        if (scored_moves[i].first > best_score) {
            k = i;
        }
    }
    std::pair<int, move16> temp = scored_moves[k];
    scored_moves[k] = scored_moves[move_index];
    scored_moves[move_index] = temp;
    move_index++;
    return temp.second;
}
