#include "see.hpp"

#include <algorithm>
#include <vector>

namespace Spotlight {

BitBoard getAttackersTo(Position &pos, int sq, BitBoard occupancy) {
    return ((knight_moves[sq] & (pos.bitboards[WHITE_KNIGHT] | pos.bitboards[BLACK_KNIGHT])) |
            (pawn_attacks[BLACK][sq] & pos.bitboards[WHITE_PAWN]) |
            (pawn_attacks[WHITE][sq] & pos.bitboards[BLACK_PAWN]) |
            (getMagicBishopAttack(sq, occupancy) &
                (pos.bitboards[WHITE_BISHOP] | pos.bitboards[WHITE_QUEEN] |
                 pos.bitboards[BLACK_BISHOP] | pos.bitboards[BLACK_QUEEN])) |
            (getMagicRookAttack(sq, occupancy) &
                (pos.bitboards[WHITE_ROOK] | pos.bitboards[WHITE_QUEEN] |
                 pos.bitboards[BLACK_ROOK] | pos.bitboards[BLACK_QUEEN])) |
            (king_moves[sq] & (pos.bitboards[WHITE_KING] | pos.bitboards[BLACK_KING]))) &
           occupancy;
}

void refreshXraysDiagonal(Position &pos, int sq, BitBoard remaining_occupancy,
                          BitBoard &attackers_bb) {
    BitBoard bishop_rays_from_to_sq =
        getMagicBishopAttack(sq, remaining_occupancy) & remaining_occupancy;
    attackers_bb |=
        bishop_rays_from_to_sq & (pos.bitboards[BLACK_BISHOP] | pos.bitboards[BLACK_QUEEN]);
    attackers_bb |=
        bishop_rays_from_to_sq & (pos.bitboards[WHITE_BISHOP] | pos.bitboards[WHITE_QUEEN]);
}

void refreshXraysRanksFiles(Position &pos, int sq, BitBoard remaining_occupancy,
                            BitBoard &attackers_bb) {
    BitBoard rook_rays_from_to_sq =
        getMagicRookAttack(sq, remaining_occupancy) & remaining_occupancy;
    attackers_bb |= rook_rays_from_to_sq & (pos.bitboards[BLACK_ROOK] | pos.bitboards[BLACK_QUEEN]);
    attackers_bb |= rook_rays_from_to_sq & (pos.bitboards[WHITE_ROOK] | pos.bitboards[WHITE_QUEEN]);
}

// Static exchange evaluation for move ordering
int see(Position &pos, move16 move) {
    Square to_sq = getToSquare(move);
    move16 move_type = getMoveType(move);
    Color side = pos.side_to_move;

    if (!isCaptureOrPromotion(move)) return 1;

    BitBoard occupancy = pos.bitboards[OCCUPANCY];

    int capture_scores[32];

    if (move_type & PROMOTION_FLAG) {
        capture_scores[0] =
            SEE_VALUES[pos.at(to_sq)] + SEE_VALUES[promoPiece(move_type)] - SEE_VALUES[PAWN];
    } else if (move_type == EN_PASSANT_CAPTURE) {
        capture_scores[0] = SEE_VALUES[PAWN];
        occupancy ^= setBit(prevPawnSquare(to_sq, side));
    } else {
        capture_scores[0] = SEE_VALUES[pos.at(to_sq)];
    }

    BitBoard attackers_bb = getAttackersTo(pos, to_sq, occupancy);

    int i = 1;
    while (true) {
        // get the least valuable attacker
        BitBoard piece_bb;
        if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(PAWN, side)])) {
            capture_scores[i] = SEE_VALUES[PAWN] - capture_scores[i - 1];
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysDiagonal(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(KNIGHT, side)])) {
            capture_scores[i] = SEE_VALUES[KNIGHT] - capture_scores[i - 1];
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(BISHOP, side)])) {
            capture_scores[i] = SEE_VALUES[BISHOP] - capture_scores[i - 1];
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysDiagonal(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(ROOK, side)])) {
            capture_scores[i] = SEE_VALUES[ROOK] - capture_scores[i - 1];
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysRanksFiles(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(QUEEN, side)])) {
            capture_scores[i] = SEE_VALUES[QUEEN] - capture_scores[i - 1];
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysDiagonal(pos, to_sq, occupancy, attackers_bb);
            refreshXraysRanksFiles(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(KING, side)])) {
            capture_scores[i] = SEE_VALUES[KING] - capture_scores[i - 1];
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
        } else {
            break;
        }

        attackers_bb ^= piece_bb;
        i++;
        side = getOtherSide(side);
    }

    for (i -= 2; i > 0; i--) {
        capture_scores[i - 1] = std::min(capture_scores[i - 1], -capture_scores[i]);
    }

    return (capture_scores[0] + SEE_MARGIN) * SEE_MULTIPLIER;
}

// Boolean SEE for pruning. The boolean form allows for early returns.
bool seeGe(Position &pos, move16 move, int margin) {
    Square from_sq = getFromSquare(move);
    Square to_sq = getToSquare(move);
    move16 move_type = getMoveType(move);

    if (isCastleMove(move_type)) return true;

    BitBoard occupancy = pos.bitboards[OCCUPANCY];

    int swap_value;

    if (move_type & PROMOTION_FLAG) {
        swap_value =
            SEE_VALUES[pos.at(to_sq)] + SEE_VALUES[promoPiece(move_type)] - SEE_VALUES[PAWN];
    } else if (move_type == EN_PASSANT_CAPTURE) {
        swap_value = SEE_VALUES[PAWN];
        occupancy ^= setBit(prevPawnSquare(to_sq, pos.side_to_move));
    } else {
        swap_value = SEE_VALUES[pos.at(to_sq)];
    }

    if (swap_value < margin) return false;

    if (move_type & PROMOTION_FLAG) {
        swap_value = SEE_VALUES[promoPiece(move_type)] - swap_value;
    } else {
        swap_value = SEE_VALUES[pos.at(from_sq)] - swap_value;
    }

    if (-swap_value >= margin) return true;

    occupancy ^= setBit(from_sq);

    BitBoard attackers_bb = getAttackersTo(pos, to_sq, occupancy);
    Color side = getOtherSide(pos.side_to_move);

    while (true) {
        // get the least valuable attacker
        BitBoard piece_bb;
        if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(PAWN, side)])) {
            swap_value = SEE_VALUES[PAWN] - swap_value;
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysDiagonal(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(KNIGHT, side)])) {
            swap_value = SEE_VALUES[KNIGHT] - swap_value;
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(BISHOP, side)])) {
            swap_value = SEE_VALUES[BISHOP] - swap_value;
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysDiagonal(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(ROOK, side)])) {
            swap_value = SEE_VALUES[ROOK] - swap_value;
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysRanksFiles(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(QUEEN, side)])) {
            swap_value = SEE_VALUES[QUEEN] - swap_value;
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
            refreshXraysDiagonal(pos, to_sq, occupancy, attackers_bb);
            refreshXraysRanksFiles(pos, to_sq, occupancy, attackers_bb);
        } else if ((piece_bb = attackers_bb & pos.bitboards[getPieceID(KING, side)])) {
            swap_value = SEE_VALUES[KING] - swap_value;
            piece_bb = lsb(piece_bb);
            occupancy ^= piece_bb;
        } else {
            return side != pos.side_to_move;
        }

        // early return if we know we the max/min possible score is below/above our margin
        if (side != pos.side_to_move && swap_value < margin) {
            return false;
        } else if (side == pos.side_to_move && -swap_value >= margin) {
            return true;
        }

        attackers_bb ^= piece_bb;
        side = getOtherSide(side);
    }

    return true;
}

}  // namespace Spotlight
