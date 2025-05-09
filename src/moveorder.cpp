#include "moveorder.hpp"
#include "moparams.hpp"

#include <vector>
#include <algorithm>

U64 getAllAttackers(Position &pos, int sq) {
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
    U64 attackers_bb = getAllAttackers(pos, to_sq);
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

    return (capture_scores[0] + SEE_MARGIN) * SEE_MULTIPLIER;
}
