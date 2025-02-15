#include "moveorder.hpp"

#include <vector>
#include <algorithm>

U64 getAllAttacks(Position &pos, int sq) {
    U64 attackers = 0ULL;

    attackers |= knight_moves[sq] & (pos.bitboards[white_knight] | pos.bitboards[black_knight]);
    attackers |= pawn_attacks[BLACK][sq] & pos.bitboards[white_pawn];
    attackers |= pawn_attacks[WHITE][sq] & pos.bitboards[black_pawn];
    U64 bishop_rays_from_sq = getMagicBishopAttack(sq, pos.bitboards[occupancy]);
    attackers |= bishop_rays_from_sq & (pos.bitboards[white_bishop] | pos.bitboards[white_queen]);
    attackers |= bishop_rays_from_sq & (pos.bitboards[black_bishop] | pos.bitboards[black_queen]);
    U64 rook_rays_from_sq = getMagicRookAttack(sq, pos.bitboards[occupancy]);
    attackers |= rook_rays_from_sq & (pos.bitboards[white_rook] | pos.bitboards[white_queen]);
    attackers |= rook_rays_from_sq & (pos.bitboards[black_rook] | pos.bitboards[black_queen]);
    attackers |= king_moves[sq] & (pos.bitboards[white_king] | pos.bitboards[black_king]);

    return attackers;
}

void refreshXraysDiagonal(Position &pos, int sq, U64 remaining_occupancy, U64 &attackers_bb) {
    U64 bishop_rays_from_to_sq = getMagicBishopAttack(sq, remaining_occupancy);
    attackers_bb |= bishop_rays_from_to_sq & (pos.bitboards[black_bishop] | pos.bitboards[black_queen]);
    attackers_bb |= bishop_rays_from_to_sq & (pos.bitboards[white_bishop] | pos.bitboards[white_queen]);
    attackers_bb &= remaining_occupancy;
}

void refreshXraysRanksFiles(Position &pos, int sq, U64 remaining_occupancy, U64 &attackers_bb) {
    U64 rook_rays_from_to_sq = getMagicRookAttack(sq, remaining_occupancy);
    attackers_bb |= rook_rays_from_to_sq & (pos.bitboards[black_rook] | pos.bitboards[black_queen]);
    attackers_bb |= rook_rays_from_to_sq & (pos.bitboards[white_rook] | pos.bitboards[white_queen]);
    attackers_bb &= remaining_occupancy;
}

// Static exchange evaluation for move ordering
int see(Position &pos, move16 move) {
    int side = pos.side_to_move ^ 1;
    int to_sq = getToSquare(move);
    int from_sq = getFromSquare(move);
    int move_type = getMoveType(move);
    U64 attackers_bb = getAllAttacks(pos, to_sq);
    U64 remaining_occupancy = pos.bitboards[occupancy];
    int capture_scores[32];
    int attacking_piece;
    if (move_type == en_passant_capture) {
        attacking_piece = BLACK_PAWN - side * BLACK_PAWN;
        capture_scores[0] = see_values[PAWN];
    } else {
        attacking_piece = pos.at(from_sq);
        capture_scores[0] = see_values[pos.at(to_sq) % 6];
    }
    assert(attacking_piece != NO_PIECE);
    
    // if this is a promotion consider the additional change in material
    if (move_type & promotion_flag) {
        switch (move_type)
        {
        case queen_promotion_capture:
            capture_scores[0] += see_values[QUEEN] - see_values[PAWN];
            attacking_piece = WHITE_QUEEN + BLACK_PAWN * side;
            break;
        case knight_promotion_capture:
            capture_scores[0] += see_values[KNIGHT] - see_values[PAWN];
            attacking_piece = WHITE_KNIGHT + BLACK_PAWN * side;
            break;
        case bishop_promotion_capture:
            capture_scores[0] += see_values[BISHOP] - see_values[PAWN];
            attacking_piece = WHITE_BISHOP + BLACK_PAWN * side;
            break;
        case rook_promotion_capture:
            capture_scores[0] += see_values[ROOK] - see_values[PAWN];
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
    U64 diagonal_xray_pieces = pos.bitboards[white_pawn] | pos.bitboards[black_pawn] | pos.bitboards[white_bishop] |
                               pos.bitboards[black_bishop] | pos.bitboards[white_queen] | pos.bitboards[black_queen];
    U64 vertical_xray_pieces = pos.bitboards[white_rook] | pos.bitboards[black_rook] | pos.bitboards[white_queen] |
                               pos.bitboards[black_queen];

    capture_scores[1] = see_values[attacking_piece % 6] - capture_scores[0];
    if (move_type == en_passant_capture) {
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
        for (int p = white_pawn + black_pawn * side; p <= white_king + black_pawn * side; p++) {
            if (pos.bitboards[p] & attackers_bb) {
                attacking_piece = p;
                current_attacker_bb = (pos.bitboards[p] & attackers_bb) & -(pos.bitboards[p] & attackers_bb);
                break;
            }
        }
        if (!current_attacker_bb) {
            break;
        }

        capture_scores[k] = see_values[attacking_piece % 6] - capture_scores[k - 1];
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

    return capture_scores[0] * 1000;
}

int scoreMove(Position &pos, move16 move) {
    int move_type = getMoveType(move);
    switch (move_type)
    {
    case capture_move:
        return see(pos, move);
        break;
    case quiet_move:
        return 0;
        // return pos.history_table[pos.side_to_move][getFromSquare(move)][getToSquare(move)];
        break;
    case queen_promotion:
        return 400000;
        // return pos.history_table[pos.side_to_move][getFromSquare(move)][getToSquare(move)] + 400000;
        break;
    case rook_promotion:
        return IGNORE_MOVE;
        break;
    case bishop_promotion:
        return IGNORE_MOVE;
        break;
    case knight_promotion:
        return 0;
        // return pos.history_table[pos.side_to_move][getFromSquare(move)][getToSquare(move)];
    case queen_promotion_capture:
        return see(pos, move);
        break;
    case rook_promotion_capture:
        return IGNORE_MOVE;
        break;
    case bishop_promotion_capture:
        return IGNORE_MOVE;
        break;
    case knight_promotion_capture:
        return see(pos, move);
    case en_passant_capture:
        return see(pos, move);
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
            sorted_moves[i] = {5001 + scoreMove(pos, moves[i]), moves[i]};
        } else if (moves[i] == killer_2) {
            sorted_moves[i] = {5000 + scoreMove(pos, moves[i]), moves[i]};
        } else {
            sorted_moves[i] = {scoreMove(pos, moves[i]), moves[i]};
        }
    }

    std::sort(sorted_moves.begin(), sorted_moves.end());

    for (int i = 0; i < s; i++) {
        moves[s - i - 1] = sorted_moves[i].second;
    }
}
