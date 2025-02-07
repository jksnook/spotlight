#include "moveorder.hpp"

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
    int to_sq = getToSquare(move);
    int from_sq = getFromSquare(move);
    U64 attackers_bb = getAllAttacks(pos, to_sq);
    U64 remaining_occupancy = pos.bitboards[occupancy];
    int attacking_piece = pos.at(from_sq);
    assert(attacking_piece != NO_PIECE);
    int side = pos.side_to_move ^ 1;
    
    // array of intermediate exchange scores
    int capture_scores[32];
    capture_scores[0] = see_values[pos.at(to_sq) % 6];
    //std::cout << capture_scores[0] << std::endl;

    // if the enemy can't recapture we simply return the score of the captured piece
    if (!(attackers_bb & pos.bitboards[white_occupancy + side])) {
        return capture_scores[0];
    }

    // simulate the initial capture
    U64 current_attacker_bb = 1ULL << from_sq;
    attackers_bb &= ~current_attacker_bb;
    remaining_occupancy &= ~current_attacker_bb;

    U64 diagonal_xray_pieces = pos.bitboards[white_pawn] | pos.bitboards[black_pawn] | pos.bitboards[white_bishop] | pos.bitboards[black_bishop];
    diagonal_xray_pieces |= pos.bitboards[white_queen] | pos.bitboards[black_queen];
    U64 vertical_xray_pieces = pos.bitboards[white_rook] | pos.bitboards[black_rook] | pos.bitboards[white_queen] | pos.bitboards[black_queen];

    capture_scores[1] = see_values[attacking_piece % 6] - capture_scores[0];
    //std::cout << capture_scores[1] << std::endl;
    if (current_attacker_bb & diagonal_xray_pieces) refreshXraysDiagonal(pos, to_sq, remaining_occupancy, attackers_bb);
    if (current_attacker_bb & vertical_xray_pieces) refreshXraysRanksFiles(pos, to_sq, remaining_occupancy, attackers_bb);
    //printBitboard(attackers_bb);

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
        //std::cout << attacking_piece << " " << capture_scores[k] << std::endl;
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
    //std::cout << k << std::endl;
    for (k -= 2;k > 0; k--) {
        capture_scores[k - 1] = -std::max(-capture_scores[k - 1], capture_scores[k]);
    }

    return capture_scores[0];
}

int scoreMove(Position &pos, move16 move) {
    int move_type = getMoveType(move);
    switch (move_type)
    {
    case capture_move:
        return see(pos, move);
        // return piece_values[pos.at(getToSquare(move)) % 6] - piece_values[pos.at(getFromSquare(move)) % 6] + 500;
        break;
    case queen_promotion:
        return 400;
        break;
    case rook_promotion:
        return -1;
        break;
    case bishop_promotion:
        return -1;
        break;
    case knight_promotion:
        return 0;
    case queen_promotion_capture:
        return 1000;
        break;
    case rook_promotion_capture:
        return -1;
        break;
    case bishop_promotion_capture:
        return -1;
        break;
    case knight_promotion_capture:
        return 0;
    case en_passant_capture:
        return 50;
    default:
        return 0;
        break;
    }
}

void orderMoves(Position &pos, MoveList &moves, move16 tt_move) {
    int s = moves.size();
    std::vector<std::pair<int, move16>> sorted_moves(s);

    for (int i = 0; i < s; i++) {
        if (moves[i] == tt_move) {
            sorted_moves[i] = {2000, moves[i]};
        } else {
            sorted_moves[i] = {scoreMove(pos, moves[i]), moves[i]};
        }
    }

    std::sort(sorted_moves.begin(), sorted_moves.end());

    for (int i = 0; i < s; i++) {
        moves[s - i - 1] = sorted_moves[i].second;
    }
}
