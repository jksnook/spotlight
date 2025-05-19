#include "movegen.hpp"

namespace Spotlight {

BitBoard getEnemyAttacks(Position &pos, Square sq) {
    BitBoard attackers = 0ULL;

    Color other_side = getOtherSide(pos.side_to_move);

    attackers |= knight_moves[sq] & (pos.bitboards[getPieceID(KNIGHT, other_side)]);
    attackers |= pawn_attacks[pos.side_to_move][sq] & pos.bitboards[getPieceID(PAWN, other_side)];
    BitBoard bishop_rays_from_sq = getMagicBishopAttack(sq, pos.bitboards[OCCUPANCY]);
    attackers |= bishop_rays_from_sq & (pos.bitboards[getPieceID(BISHOP, other_side)] |
                                        pos.bitboards[getPieceID(QUEEN, other_side)]);
    BitBoard rook_rays_from_sq = getMagicRookAttack(sq, pos.bitboards[OCCUPANCY]);
    attackers |= rook_rays_from_sq & (pos.bitboards[getPieceID(ROOK, other_side)] |
                                      pos.bitboards[getPieceID(QUEEN, other_side)]);
    attackers |= king_moves[sq] & pos.bitboards[getPieceID(KING, other_side)];

    return attackers;
}

template <Color side>
BitBoard getCheckers(Position &pos, int king_index) {
    constexpr Color enemy_side = getOtherSide(side);

    BitBoard checkers = 0ULL;

    checkers |= knight_moves[king_index] & pos.bitboards[getPieceID(KNIGHT, enemy_side)];
    checkers |= pawn_attacks[side][king_index] & pos.bitboards[getPieceID(PAWN, enemy_side)];
    BitBoard bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
    BitBoard checking_bishops =
        bishop_rays_from_king & (pos.bitboards[getPieceID(BISHOP, enemy_side)] |
                                 pos.bitboards[getPieceID(QUEEN, enemy_side)]);
    checkers |= checking_bishops;
    BitBoard rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
    BitBoard checking_rooks = rook_rays_from_king & (pos.bitboards[getPieceID(ROOK, enemy_side)] |
                                                     pos.bitboards[getPieceID(QUEEN, enemy_side)]);
    checkers |= checking_rooks;
    // king check detection needed for pseudolegal moves
    checkers |= king_moves[king_index] & pos.bitboards[getPieceID(KING, enemy_side)];

    return checkers;
}

template <Color side>
BitBoard getAllEnemyAttacks(Position &pos) {
    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const Color enemy_side = getOtherSide(side);

    // find all squares attacked by the opponent
    BitBoard enemy_attacks =
        pawnAttacksFromBitboard<getOtherSide(side)>(pos.bitboards[getPieceID(PAWN, enemy_side)]);
    enemy_attacks |= knightAttacksFromBitboard(pos.bitboards[getPieceID(KNIGHT, enemy_side)]);
    // treat the king as invisible for bishops and rooks so we know which squares we can't move to
    enemy_attacks |= bishopAttacksFromBitboard(
        pos.bitboards[getPieceID(BISHOP, enemy_side)] |
            pos.bitboards[getPieceID(QUEEN, enemy_side)],
        pos.bitboards[OCCUPANCY] & ~pos.bitboards[getPieceID(KING, side)]);
    enemy_attacks |= rookAttacksFromBitboard(
        pos.bitboards[getPieceID(ROOK, enemy_side)] | pos.bitboards[getPieceID(QUEEN, enemy_side)],
        pos.bitboards[OCCUPANCY] & ~pos.bitboards[getPieceID(KING, side)]);
    enemy_attacks |= king_moves[bitScanForward(pos.bitboards[getPieceID(KING, enemy_side)])];

    return enemy_attacks;
}

template <Color side, GenType gen_type>
void generateMovesSided(MoveList &moves, Position &pos) {
    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const Color enemy_side = getOtherSide(side);

    constexpr const Piece friendly_pawn = getPieceID(PAWN, side);
    constexpr const Piece friendly_knight = getPieceID(KNIGHT, side);
    constexpr const Piece friendly_bishop = getPieceID(BISHOP, side);
    constexpr const Piece friendly_rook = getPieceID(ROOK, side);
    constexpr const Piece friendly_queen = getPieceID(QUEEN, side);
    constexpr const Piece friendly_king = getPieceID(KING, side);

    constexpr const Piece enemy_pawn = getPieceID(PAWN, enemy_side);
    constexpr const Piece enemy_knight = getPieceID(KNIGHT, enemy_side);
    constexpr const Piece enemy_bishop = getPieceID(BISHOP, enemy_side);
    constexpr const Piece enemy_rook = getPieceID(ROOK, enemy_side);
    constexpr const Piece enemy_queen = getPieceID(QUEEN, enemy_side);
    constexpr const Piece enemy_king = getPieceID(KING, enemy_side);

    constexpr const int friendly_occupancy = getOccupancy(side);
    constexpr const int enemy_occupancy = getOccupancy(enemy_side);

    // find all squares attacked by the opponent
    BitBoard enemy_attacks;
    if (pos.movegen_data.generated_enemy_attacks) {
        enemy_attacks = pos.movegen_data.enemy_attacks;
    } else {
        enemy_attacks = getAllEnemyAttacks<side>(pos);
        pos.movegen_data.generated_enemy_attacks = true;
        pos.movegen_data.enemy_attacks = enemy_attacks;
    }

    // find the checks on the king and create attack and block masks
    Square king_index = bitScanForward(pos.bitboards[friendly_king]);
    BitBoard checkers = 0ULL;

    BitBoard block_mask = 0ULL;

    if (pos.movegen_data.generated_checkers) {
        checkers = pos.movegen_data.checkers;

        if (checkers) {
            BitBoard bishop_rays_from_king =
                getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
            BitBoard rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
            BitBoard checking_bishops = checkers & bishop_rays_from_king &
                                        (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
            BitBoard checking_rooks = checkers & rook_rays_from_king &
                                      (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);

            block_mask = bishopAttacksFromBitboard(checking_bishops, pos.bitboards[OCCUPANCY]) &
                         bishop_rays_from_king;
            block_mask |= rookAttacksFromBitboard(checking_rooks, pos.bitboards[OCCUPANCY]) &
                          rook_rays_from_king;
        }
    } else {
        checkers |= knight_moves[king_index] & pos.bitboards[enemy_knight];
        checkers |= pawn_attacks[side][king_index] & pos.bitboards[enemy_pawn];
        BitBoard bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
        BitBoard checking_bishops =
            bishop_rays_from_king & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
        checkers |= checking_bishops;
        BitBoard rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
        BitBoard checking_rooks =
            rook_rays_from_king & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);
        checkers |= checking_rooks;
        // king check detection needed for pseudolegal moves
        checkers |= king_moves[king_index] & pos.bitboards[enemy_king];

        block_mask |= bishopAttacksFromBitboard(checking_bishops, pos.bitboards[OCCUPANCY]) &
                      bishop_rays_from_king;
        block_mask |=
            rookAttacksFromBitboard(checking_rooks, pos.bitboards[OCCUPANCY]) & rook_rays_from_king;

        pos.movegen_data.generated_checkers = true;
        pos.movegen_data.checkers = checkers;
    }

    int num_checks = countBits(checkers);

    if (num_checks > 0) {
        pos.in_check = true;
    } else {
        pos.in_check = false;
    }

    // generate king moves
    if constexpr (gen_type == QUIET || gen_type == LEGAL) {
        // add quiet moves
        addMovesFromBitboard(king_index,
                             king_moves[king_index] & ~enemy_attacks & ~pos.bitboards[OCCUPANCY],
                             QUIET_MOVE, moves);
    }
    if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
        // add captures
        addMovesFromBitboard(
            king_index, king_moves[king_index] & ~enemy_attacks & pos.bitboards[enemy_occupancy],
            CAPTURE_MOVE, moves);
    }

    // early return if double check
    if (num_checks >= 2) {
        return;
    }

    // capture mask for when we are in check
    BitBoard capture_mask = checkers;

    // If we are not in check the capture and block masks should be all ones
    if (num_checks == 0) {
        block_mask = ~0ULL;
        capture_mask = ~0ULL;
    }

    // generate moves for pinned pieces

    // pinned on diagonals
    BitBoard blockers =
        getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]) & pos.bitboards[OCCUPANCY];
    BitBoard xray = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY] & ~blockers);
    BitBoard pinners = xray & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);

    BitBoard pin_rays =
        bishopAttacksFromBitboard(pinners, pos.bitboards[OCCUPANCY] & ~blockers) & xray;

    BitBoard pinned_pieces = pin_rays & pos.bitboards[friendly_occupancy];

    // moves for pinned pawns on diagonals

    // promotions first
    BitBoard pinned_pawns;

    if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
        if constexpr (side == WHITE) {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & RANK_7;
        } else {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & RANK_2;
        }

        while (pinned_pawns) {
            // promotion captures
            Square piece_index = popLSB(pinned_pawns);
            BitBoard moves_bb = pawn_attacks[side][piece_index] & pinners & capture_mask;
            addMovesFromBitboard(piece_index, moves_bb, QUEEN_PROMOTION_CAPTURE, moves);
            addMovesFromBitboard(piece_index, moves_bb, ROOK_PROMOTION_CAPTURE, moves);
            addMovesFromBitboard(piece_index, moves_bb, BISHOP_PROMOTION_CAPTURE, moves);
            addMovesFromBitboard(piece_index, moves_bb, KNIGHT_PROMOTION_CAPTURE, moves);
        }

        if constexpr (side == WHITE) {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & ~RANK_7;
        } else {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & ~RANK_2;
        }

        while (pinned_pawns) {
            // normal captures
            Square piece_index = popLSB(pinned_pawns);
            addMovesFromBitboard(piece_index,
                                 pawn_attacks[side][piece_index] & pinners & capture_mask,
                                 CAPTURE_MOVE, moves);
            // en passant
            if (pos.en_passant) {
                addMovesFromBitboard(piece_index,
                                     pawn_attacks[side][piece_index] & (1ULL << pos.en_passant) &
                                         pin_rays & capture_mask,
                                     EN_PASSANT_CAPTURE, moves);
            }
        }
    }

    // moves for bishops and queens pinned on diagonals

    BitBoard pinned_bishops_and_queens =
        pinned_pieces & (pos.bitboards[friendly_bishop] | pos.bitboards[friendly_queen]);
    BitBoard magic_attack;

    while (pinned_bishops_and_queens) {
        Square piece_index = popLSB(pinned_bishops_and_queens);
        magic_attack = getMagicBishopAttack(piece_index, pos.bitboards[OCCUPANCY]);
        // quiet moves
        if constexpr (gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pin_rays & block_mask, QUIET_MOVE,
                                 moves);
        }
        // captures
        if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pinners & capture_mask, CAPTURE_MOVE,
                                 moves);
        }
    }

    // pinned on ranks and files

    blockers = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]) & pos.bitboards[OCCUPANCY];
    xray = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY] & ~blockers);
    pinners = xray & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);

    pin_rays = rookAttacksFromBitboard(pinners, pos.bitboards[OCCUPANCY] & ~blockers) & xray;

    BitBoard pinned_on_rank_and_file = pin_rays & pos.bitboards[friendly_occupancy];
    pinned_pieces |= pinned_on_rank_and_file;

    // pinned pawns on files
    if constexpr (gen_type == QUIET || gen_type == LEGAL) {
        pinned_pawns = pinned_on_rank_and_file & pos.bitboards[friendly_pawn];

        while (pinned_pawns) {
            Square piece_index = popLSB(pinned_pawns);
            addMovesFromBitboard(piece_index,
                                 pawn_pushes[side][piece_index] & pin_rays & block_mask, QUIET_MOVE,
                                 moves);
            if constexpr (side == WHITE) {
                addMovesFromBitboard(piece_index,
                                     pawn_double_pushes[side][piece_index] & RANK_4 & pin_rays &
                                         block_mask & ~pos.bitboards[OCCUPANCY] << 8,
                                     DOUBLE_PAWN_PUSH, moves);
            } else {
                addMovesFromBitboard(piece_index,
                                     pawn_double_pushes[side][piece_index] & RANK_5 & pin_rays &
                                         block_mask & ~pos.bitboards[OCCUPANCY] >> 8,
                                     DOUBLE_PAWN_PUSH, moves);
            }
        }
    }

    // pinned rooks and queens
    BitBoard pinned_rooks_and_queens =
        pinned_on_rank_and_file & (pos.bitboards[friendly_rook] | pos.bitboards[friendly_queen]);

    while (pinned_rooks_and_queens) {
        Square piece_index = popLSB(pinned_rooks_and_queens);
        magic_attack = getMagicRookAttack(piece_index, pos.bitboards[OCCUPANCY]);
        // quiet moves
        if constexpr (gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & block_mask & pin_rays, QUIET_MOVE,
                                 moves);
        }
        // captures
        if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pinners & capture_mask, CAPTURE_MOVE,
                                 moves);
        }
    }

    // knight moves
    BitBoard knights = pos.bitboards[friendly_knight] & ~pinned_pieces;

    while (knights) {
        Square piece_index = popLSB(knights);
        if constexpr (gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index,
                                 knight_moves[piece_index] & ~pos.bitboards[OCCUPANCY] & block_mask,
                                 QUIET_MOVE, moves);
        }
        if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
            addMovesFromBitboard(
                piece_index,
                knight_moves[piece_index] & pos.bitboards[enemy_occupancy] & capture_mask,
                CAPTURE_MOVE, moves);
        }
    }

    // pawn moves
    BitBoard pawns = pos.bitboards[friendly_pawn] & ~pinned_pieces;

    BitBoard double_pushes;
    BitBoard single_pushes;
    BitBoard left_attacks;
    BitBoard right_attacks;
    if constexpr (side == WHITE) {
        single_pushes = pawns << 8 & block_mask & ~pos.bitboards[OCCUPANCY];
        if constexpr (gen_type == QUIET || gen_type == LEGAL) {
            double_pushes = (pawns & RANK_2) << 16 & block_mask & ~pos.bitboards[OCCUPANCY] &
                            ~pos.bitboards[OCCUPANCY] << 8;
        }
        if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
            left_attacks = pawns << 7 & pos.bitboards[enemy_occupancy] & capture_mask & ~H_FILE;
            right_attacks = pawns << 9 & pos.bitboards[enemy_occupancy] & capture_mask & ~A_FILE;
        }
    } else {
        single_pushes = pawns >> 8 & block_mask & ~pos.bitboards[OCCUPANCY];
        if constexpr (gen_type == QUIET || gen_type == LEGAL) {
            double_pushes = (pawns & RANK_7) >> 16 & block_mask & ~pos.bitboards[OCCUPANCY] &
                            ~pos.bitboards[OCCUPANCY] >> 8;
        }
        if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
            left_attacks = pawns >> 9 & pos.bitboards[enemy_occupancy] & capture_mask & ~H_FILE;
            right_attacks = pawns >> 7 & pos.bitboards[enemy_occupancy] & capture_mask & ~A_FILE;
        }
    }

    BitBoard promotions;
    BitBoard left_promotion_captures;
    BitBoard right_promotion_captures;

    promotions = single_pushes & (RANK_8 | RANK_1);

    if constexpr (gen_type == QUIET || gen_type == LEGAL) {
        single_pushes = single_pushes & ~promotions;
    }
    if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
        left_promotion_captures = left_attacks & (RANK_8 | RANK_1);
        left_attacks = left_attacks & ~left_promotion_captures;
        right_promotion_captures = right_attacks & (RANK_8 | RANK_1);
        right_attacks = right_attacks & ~right_promotion_captures;
    }

    Square start_square;
    Square end_square;

    if constexpr (gen_type == QUIET || gen_type == LEGAL) {
        while (double_pushes) {
            end_square = popLSB(double_pushes);
            if constexpr (side == WHITE) {
                start_square = static_cast<Square>(end_square - 16);
            } else {
                start_square = static_cast<Square>(end_square + 16);
            }
            moves.addMove(encodeMove(start_square, end_square, DOUBLE_PAWN_PUSH));
        }

        while (single_pushes) {
            end_square = popLSB(single_pushes);
            if constexpr (side == WHITE) {
                start_square = static_cast<Square>(end_square - 8);
            } else {
                start_square = static_cast<Square>(end_square + 8);
            }
            moves.addMove(encodeMove(start_square, end_square, QUIET_MOVE));
        }
    }

    if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
        while (promotions) {
            end_square = popLSB(promotions);
            if constexpr (side == WHITE) {
                start_square = static_cast<Square>(end_square - 8);
            } else {
                start_square = static_cast<Square>(end_square + 8);
            }
            moves.addMove(encodeMove(start_square, end_square, QUEEN_PROMOTION));
            moves.addMove(encodeMove(start_square, end_square, KNIGHT_PROMOTION));
            moves.addMove(encodeMove(start_square, end_square, BISHOP_PROMOTION));
            moves.addMove(encodeMove(start_square, end_square, ROOK_PROMOTION));
        }

        while (left_promotion_captures) {
            end_square = popLSB(left_promotion_captures);
            if constexpr (side == WHITE) {
                start_square = static_cast<Square>(end_square - 7);
            } else {
                start_square = static_cast<Square>(end_square + 9);
            }
            moves.addMove(encodeMove(start_square, end_square, QUEEN_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, KNIGHT_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, BISHOP_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, ROOK_PROMOTION_CAPTURE));
        }

        while (right_promotion_captures) {
            end_square = popLSB(right_promotion_captures);
            if constexpr (side == WHITE) {
                start_square = static_cast<Square>(end_square - 9);
            } else {
                start_square = static_cast<Square>(end_square + 7);
            }
            moves.addMove(encodeMove(start_square, end_square, QUEEN_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, KNIGHT_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, BISHOP_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, ROOK_PROMOTION_CAPTURE));
        }

        while (left_attacks) {
            end_square = popLSB(left_attacks);
            if constexpr (side == WHITE) {
                start_square = static_cast<Square>(end_square - 7);
            } else {
                start_square = static_cast<Square>(end_square + 9);
            }
            moves.addMove(encodeMove(start_square, end_square, CAPTURE_MOVE));
        }

        while (right_attacks) {
            end_square = popLSB(right_attacks);
            if constexpr (side == WHITE) {
                start_square = static_cast<Square>(end_square - 9);
            } else {
                start_square = static_cast<Square>(end_square + 7);
            }
            moves.addMove(encodeMove(start_square, end_square, CAPTURE_MOVE));
        }

        // en passant
        if (pos.en_passant) {
            BitBoard en_passant_attackers;
            Square piece_index;
            en_passant_attackers = pawn_attacks[enemy_side][pos.en_passant] & pawns;
            if constexpr (side == WHITE) {
                piece_index = static_cast<Square>(pos.en_passant - 8);
            } else {
                piece_index = static_cast<Square>(pos.en_passant + 8);
            }
            if ((num_checks == 0) ||
                (pawn_attacks[enemy_side][piece_index] & pos.bitboards[friendly_king])) {
                BitBoard ep_rank;
                if constexpr (side == WHITE) {
                    ep_rank = RANK_5;
                } else {
                    ep_rank = RANK_4;
                }
                while (en_passant_attackers) {
                    start_square = popLSB(en_passant_attackers);
                    if (pos.bitboards[friendly_king] & ep_rank) {
                        // check to see if the move leaves the king in check
                        BitBoard ep_pin_rays = getMagicRookAttack(
                            king_index, pos.bitboards[OCCUPANCY] & ~setBit(start_square) &
                                            ~setBit(piece_index));
                        if (ep_pin_rays &
                            (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen])) {
                            break;
                        }
                    }
                    moves.addMove(encodeMove(start_square, pos.en_passant, EN_PASSANT_CAPTURE));
                }
            }
        }
    }

    // sliding piece moves

    // bishop moves (including queens)

    BitBoard bishops =
        (pos.bitboards[friendly_bishop] | pos.bitboards[friendly_queen]) & ~pinned_pieces;

    while (bishops) {
        Square piece_index = popLSB(bishops);
        magic_attack = getMagicBishopAttack(piece_index, pos.bitboards[OCCUPANCY]);
        if constexpr (gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & ~pos.bitboards[OCCUPANCY] & block_mask,
                                 QUIET_MOVE, moves);
        }
        if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index,
                                 magic_attack & pos.bitboards[enemy_occupancy] & capture_mask,
                                 CAPTURE_MOVE, moves);
        }
    }

    // rook moves (including queens)

    BitBoard rooks =
        (pos.bitboards[friendly_rook] | pos.bitboards[friendly_queen]) & ~pinned_pieces;

    while (rooks) {
        Square piece_index = popLSB(rooks);
        magic_attack = getMagicRookAttack(piece_index, pos.bitboards[OCCUPANCY]);
        if constexpr (gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & ~pos.bitboards[OCCUPANCY] & block_mask,
                                 QUIET_MOVE, moves);
        }
        if constexpr (gen_type == CAPTURES_AND_PROMOTIONS || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index,
                                 magic_attack & pos.bitboards[enemy_occupancy] & capture_mask,
                                 CAPTURE_MOVE, moves);
        }
    }

    if constexpr (gen_type == QUIET || gen_type == LEGAL) {
        // castling
        if (num_checks == 0) {
            if constexpr (side == WHITE) {
                if ((WKC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & WKC_SQUARES) &&
                    !(enemy_attacks & WKC_KING_SQUARES)) {
                    moves.addMove(encodeMove(E1, G1, KING_CASTLE));
                }
                if ((WQC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & WQC_SQUARES) &&
                    !(enemy_attacks & WQC_KING_SQUARES)) {
                    moves.addMove(encodeMove(E1, C1, QUEEN_CASTLE));
                }
            } else {
                if ((BKC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & BKC_SQUARES) &&
                    !(enemy_attacks & BKC_KING_SQUARES)) {
                    moves.addMove(encodeMove(E8, G8, KING_CASTLE));
                }
                if ((BQC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & BQC_SQUARES) &&
                    !(enemy_attacks & BQC_KING_SQUARES)) {
                    moves.addMove(encodeMove(E8, C8, QUEEN_CASTLE));
                }
            }
        }
    }
}

void generateNoisyMoves(MoveList &moves, Position &pos) {
    if (pos.side_to_move == WHITE) {
        generateMovesSided<WHITE, CAPTURES_AND_PROMOTIONS>(moves, pos);
    } else {
        generateMovesSided<BLACK, CAPTURES_AND_PROMOTIONS>(moves, pos);
    }
}

void generateQuietMoves(MoveList &moves, Position &pos) {
    if (pos.side_to_move == WHITE) {
        generateMovesSided<WHITE, QUIET>(moves, pos);
    } else {
        generateMovesSided<BLACK, QUIET>(moves, pos);
    }
}

void generateMoves(MoveList &moves, Position &pos) {
    if (pos.side_to_move == WHITE) {
        generateMovesSided<WHITE, LEGAL>(moves, pos);
    } else {
        generateMovesSided<BLACK, LEGAL>(moves, pos);
    }
}

template <Color side>
bool inCheckSided(Position &pos) {
    if (side == pos.side_to_move && pos.movegen_data.generated_checkers) {
        if (pos.movegen_data.checkers) {
            pos.in_check = true;
            return true;
        }
        return false;
    }

    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const Color enemy_side = getOtherSide(side);

    constexpr const Piece friendly_king = getPieceID(KING, side);

    constexpr const Piece enemy_pawn = getPieceID(PAWN, enemy_side);
    constexpr const Piece enemy_knight = getPieceID(KNIGHT, enemy_side);
    constexpr const Piece enemy_bishop = getPieceID(BISHOP, enemy_side);
    constexpr const Piece enemy_rook = getPieceID(ROOK, enemy_side);
    constexpr const Piece enemy_queen = getPieceID(QUEEN, enemy_side);
    constexpr const Piece enemy_king = getPieceID(KING, enemy_side);

    // find the checks on the king

    BitBoard checkers = 0ULL;
    Square king_index = bitScanForward(pos.bitboards[friendly_king]);

    checkers |= knight_moves[king_index] & pos.bitboards[enemy_knight];
    checkers |= pawn_attacks[side][king_index] & pos.bitboards[enemy_pawn];
    BitBoard bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
    BitBoard checking_bishops =
        bishop_rays_from_king & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
    checkers |= checking_bishops;
    BitBoard rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
    BitBoard checking_rooks =
        rook_rays_from_king & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);
    checkers |= checking_rooks;
    // king check detection needed for pseudolegal moves
    checkers |= king_moves[king_index] & pos.bitboards[enemy_king];

    if (side == pos.side_to_move) {
        pos.movegen_data.generated_checkers = true;
        pos.movegen_data.checkers = checkers;
    }

    if (checkers) {
        pos.in_check = true;
        return true;
    }
    return false;
}

bool inCheck(Position &pos) {
    if (pos.side_to_move == WHITE) {
        return inCheckSided<WHITE>(pos);
    } else {
        return inCheckSided<BLACK>(pos);
    }
}

bool otherSideInCheck(Position &pos) {
    if (pos.side_to_move == BLACK) {
        return inCheckSided<WHITE>(pos);
    } else {
        return inCheckSided<BLACK>(pos);
    }
}

bool isPseudoLegal(move16 move, Position &pos) {
    if (move == NULL_MOVE) {
        return false;
    }

    Square from_sq = getFromSquare(move);
    Square to_sq = getToSquare(move);
    Piece piece = pos.at(from_sq);
    PieceType piece_type = getPieceType(piece);
    if (piece == NO_PIECE || getPieceColor(piece) != pos.side_to_move) {
        return false;
    }

    int move_type = getMoveType(move);
    if (move_type == UNUSED_MOVE_TYPE_1 || move_type == UNUSED_MOVE_TYPE_2) {
        return false;
    }

    if (move_type & CAPTURE_MOVE && move_type != EN_PASSANT_CAPTURE) {
        if (!(setBit(to_sq) & pos.bitboards[BLACK_OCCUPANCY - pos.side_to_move])) return false;
    } else if (move_type != EN_PASSANT_CAPTURE) {
        if (setBit(to_sq) & pos.bitboards[OCCUPANCY]) return false;
    }

    if (piece_type == PAWN) {
        if (!(move_type & PROMOTION_FLAG) && setBit(to_sq) & (RANK_1 | RANK_8)) {
            return false;
        }
        if (move_type & CAPTURE_MOVE) {
            if (!(setBit(to_sq) & pawn_attacks[pos.side_to_move][from_sq])) return false;
        } else {
            if (!(setBit(to_sq) & (pawn_pushes[pos.side_to_move][from_sq]) ||
                  move_type == DOUBLE_PAWN_PUSH)) {
                return false;
            }
        }
    } else if (piece_type == KNIGHT) {
        if (!(setBit(to_sq) & knight_moves[from_sq])) return false;
    } else if (piece_type == BISHOP) {
        if (!(setBit(to_sq) & getMagicBishopAttack(from_sq, pos.bitboards[OCCUPANCY]))) {
            return false;
        }
    } else if (piece_type == ROOK) {
        if (!(setBit(to_sq) & getMagicRookAttack(from_sq, pos.bitboards[OCCUPANCY]))) {
            return false;
        }
    } else if (piece_type == QUEEN) {
        if (!(setBit(to_sq) & (getMagicRookAttack(from_sq, pos.bitboards[OCCUPANCY]) |
                               getMagicBishopAttack(from_sq, pos.bitboards[OCCUPANCY])))) {
            return false;
        }
    } else if (piece_type == KING && move_type != KING_CASTLE && move_type != QUEEN_CASTLE) {
        if (!(setBit(to_sq) & king_moves[from_sq])) return false;
    }

    if (move_type & PROMOTION_FLAG) {
        if (piece_type != PAWN || !(setBit(to_sq) & (RANK_1 << (56 * (pos.side_to_move ^ 1))))) {
            return false;
        }
    } else if (move_type == DOUBLE_PAWN_PUSH) {
        if (piece_type != PAWN ||
            !(setBit(to_sq) & pawn_double_pushes[pos.side_to_move][from_sq])) {
            return false;
        }
        if (pawn_pushes[pos.side_to_move][from_sq] & pos.bitboards[OCCUPANCY]) return false;
    } else if (move_type == EN_PASSANT_CAPTURE) {
        if (piece_type != PAWN || !pos.en_passant || to_sq != pos.en_passant) {
            return false;
        }
    } else if (move_type == KING_CASTLE || move_type == QUEEN_CASTLE) {
        if (piece_type != KING) return false;
        BitBoard pass_through;
        BitBoard rook_sq_bb;
        BitBoard rook_bb;
        BitBoard temp;
        if (move_type == KING_CASTLE) {
            if (!(pos.castle_rights & (WKC << (2 * pos.side_to_move)))) return false;
            if (from_sq != E1 + 56 * pos.side_to_move || to_sq != G1 + 56 * pos.side_to_move) {
                return false;
            }
            pass_through = WKC_SQUARES << 56 * pos.side_to_move;
            rook_sq_bb = setBit(H1) << 56 * pos.side_to_move;
            rook_bb = pos.bitboards[getPieceID(ROOK, pos.side_to_move)];
            temp = (setBit(E1) | setBit(F1)) << 56 * pos.side_to_move;
        } else {
            if (!(pos.castle_rights & (WQC << (2 * pos.side_to_move)))) return false;
            if (from_sq != E1 + 56 * pos.side_to_move || to_sq != C1 + 56 * pos.side_to_move) {
                return false;
            }
            pass_through = WQC_SQUARES << 56 * pos.side_to_move;
            rook_sq_bb = setBit(A1) << 56 * pos.side_to_move;
            rook_bb = pos.bitboards[getPieceID(ROOK, pos.side_to_move)];
            temp = (setBit(E1) | setBit(D1)) << 56 * pos.side_to_move;
        }
        if ((pos.bitboards[OCCUPANCY] & pass_through) || !(rook_sq_bb & rook_bb)) {
            return false;
        }
        while (temp) {
            Square sq = popLSB(temp);
            if (getEnemyAttacks(pos, sq)) {
                return false;
            }
        }
    }
    return true;
}

bool isLegal(move16 move, Position &pos) {
    if (!isPseudoLegal(move, pos)) {
        return false;
    }
    pos.makeMove(move);
    if (otherSideInCheck(pos)) {
        pos.unmakeMove();
        return false;
    }
    pos.unmakeMove();
    return true;
}

U64 perftHelper(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    if (pos.side_to_move == WHITE) {
        // generateMovesSided<WHITE, LEGAL>(moves, pos);
        generateMovesSided<WHITE, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<WHITE, QUIET>(moves, pos);
    } else {
        // generateMovesSided<BLACK, LEGAL>(moves, pos);
        generateMovesSided<BLACK, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<BLACK, QUIET>(moves, pos);
    }

    if (depth == 1) {
        return moves.size();
    }

    for (auto &sm : moves) {
        pos.makeMove(sm.move);
        nodes += perftHelper(pos, depth - 1);
        pos.unmakeMove();
    }

    return nodes;
};

U64 perft(Position &pos, int depth) {
    // std::cout << "Bulk counting disabled\n";
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    if (pos.side_to_move == WHITE) {
        // generateMovesSided<WHITE, LEGAL>(moves, pos);
        generateMovesSided<WHITE, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<WHITE, QUIET>(moves, pos);
    } else {
        // generateMovesSided<BLACK, LEGAL>(moves, pos);
        generateMovesSided<BLACK, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<BLACK, QUIET>(moves, pos);
    }

    for (auto &sm : moves) {
        pos.makeMove(sm.move);
        int nodes_this_move = perftHelper(pos, depth - 1);
        pos.unmakeMove();
        nodes += nodes_this_move;
        std::cout << moveToString(sm.move) << ": " << nodes_this_move << std::endl;
    }

    return nodes;
};

}  // namespace Spotlight
