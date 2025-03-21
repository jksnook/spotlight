#pragma once

#include <iostream>

#include "types.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "bitboards.hpp"
#include "move.hpp"

bool isPseudoLegal(move16 move, Position &pos);

bool isLegal(move16 move, Position &pos);

U64 getEnemyAttacks(Position &pos, int sq);

void generateCaptures(MoveList &moves, Position &pos);

void generateQuietMoves(MoveList &moves, Position &pos);

template <Color side>
U64 getCheckers(Position &pos, int king_index) {
    constexpr const int enemy_side = getOtherSide(side);

    U64 checkers = 0ULL;

    checkers |= knight_moves[king_index] & pos.bitboards[getPieceID(KNIGHT, enemy_side)];
    checkers |= pawn_attacks[side][king_index] & pos.bitboards[getPieceID(PAWN, enemy_side)];
    U64 bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
    U64 checking_bishops = bishop_rays_from_king & (pos.bitboards[getPieceID(BISHOP, enemy_side)] | pos.bitboards[getPieceID(QUEEN, enemy_side)]);
    checkers |= checking_bishops;
    U64 rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
    U64 checking_rooks = rook_rays_from_king & (pos.bitboards[getPieceID(ROOK, enemy_side)] | pos.bitboards[getPieceID(QUEEN, enemy_side)]);
    checkers |= checking_rooks;
    // king check detection needed for pseudolegal moves
    checkers |= king_moves[king_index] & pos.bitboards[getPieceID(KING, enemy_side)];

    return checkers;
}

template <Color side>
U64 getAllEnemyAttacks(Position &pos) {
    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const int enemy_side = getOtherSide(side);

    // find all squares attacked by the opponent 
    U64 enemy_attacks = pawnAttacksFromBitboard<getOtherSide(side)>(pos.bitboards[getPieceID(PAWN, enemy_side)]);
    enemy_attacks |= knightAttacksFromBitboard(pos.bitboards[getPieceID(KNIGHT, enemy_side)]);
    // treat the king as invisible for bishops and rooks so we know which squares we can't move to
    enemy_attacks |= bishopAttacksFromBitboard(pos.bitboards[getPieceID(BISHOP, enemy_side)] |
         pos.bitboards[getPieceID(QUEEN, enemy_side)], pos.bitboards[OCCUPANCY] & ~pos.bitboards[getPieceID(KING, side)]);
    enemy_attacks |= rookAttacksFromBitboard(pos.bitboards[getPieceID(ROOK, enemy_side)] | 
        pos.bitboards[getPieceID(QUEEN, enemy_side)], pos.bitboards[OCCUPANCY] & ~pos.bitboards[getPieceID(KING, side)]);
    enemy_attacks |= king_moves[bitScanForward(pos.bitboards[getPieceID(KING, enemy_side)])];

    return enemy_attacks;
}

template <Color side, GenType gen_type>
void generateMoves(MoveList &moves, Position &pos) {

    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const int enemy_side = getOtherSide(side);

    constexpr const int friendly_pawn = getPieceID(PAWN, side);
    constexpr const int friendly_knight = getPieceID(KNIGHT, side);
    constexpr const int friendly_bishop = getPieceID(BISHOP, side);
    constexpr const int friendly_rook = getPieceID(ROOK, side);
    constexpr const int friendly_queen = getPieceID(QUEEN, side);
    constexpr const int friendly_king = getPieceID(KING, side);

    constexpr const int enemy_pawn = getPieceID(PAWN, enemy_side);
    constexpr const int enemy_knight = getPieceID(KNIGHT, enemy_side);
    constexpr const int enemy_bishop = getPieceID(BISHOP, enemy_side);
    constexpr const int enemy_rook = getPieceID(ROOK, enemy_side);
    constexpr const int enemy_queen = getPieceID(QUEEN, enemy_side);
    constexpr const int enemy_king = getPieceID(KING, enemy_side);

    constexpr const int friendly_occupancy = getOccupancy(side);
    constexpr const int enemy_occupancy = getOccupancy(enemy_side);


    // find all squares attacked by the opponent 
    U64 enemy_attacks = getAllEnemyAttacks<side>(pos);

    // find the checks on the king and create attack and block masks
    int king_index = bitScanForward(pos.bitboards[friendly_king]);
    U64 checkers = 0ULL;

    U64 block_mask = 0ULL;

    if (pos.movegen_data.generated_checkers) {
        checkers = pos.movegen_data.checkers;

        if (checkers) {
            U64 bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
            U64 rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
            U64 checking_bishops = checkers & bishop_rays_from_king & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
            U64 checking_rooks = checkers & rook_rays_from_king & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);

            block_mask = bishopAttacksFromBitboard(checking_bishops, pos.bitboards[OCCUPANCY]) & bishop_rays_from_king;
            block_mask |= rookAttacksFromBitboard(checking_rooks, pos.bitboards[OCCUPANCY]) & rook_rays_from_king;
        }
    } else {
        checkers |= knight_moves[king_index] & pos.bitboards[enemy_knight];
        checkers |= pawn_attacks[side][king_index] & pos.bitboards[enemy_pawn];
        U64 bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
        U64 checking_bishops = bishop_rays_from_king & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
        checkers |= checking_bishops;
        U64 rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
        U64 checking_rooks = rook_rays_from_king & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);
        checkers |= checking_rooks;
        // king check detection needed for pseudolegal moves
        checkers |= king_moves[king_index] & pos.bitboards[enemy_king];

        block_mask |= bishopAttacksFromBitboard(checking_bishops, pos.bitboards[OCCUPANCY]) & bishop_rays_from_king;
        block_mask |= rookAttacksFromBitboard(checking_rooks, pos.bitboards[OCCUPANCY]) & rook_rays_from_king;

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
    if constexpr(gen_type == QUIET || gen_type == LEGAL) {
        // add quiet moves
        addMovesFromBitboard(king_index, king_moves[king_index] & ~enemy_attacks & ~pos.bitboards[OCCUPANCY], QUIET_MOVE, moves);
    }
    if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
        // add captures
        addMovesFromBitboard(king_index, king_moves[king_index] & ~enemy_attacks & pos.bitboards[enemy_occupancy], CAPTURE_MOVE, moves);
    }

    // early return if double check
    if (num_checks >= 2) {
        return;
    }
    
    // capture mask for when we are in check
    U64 capture_mask = checkers;

    // If we are not in check the capture and block masks should be all ones
    if (num_checks == 0) {
        block_mask = ~0ULL;
        capture_mask = ~0ULL;
    }

    // generate moves for pinned pieces

    // pinned on diagonals
    U64 blockers = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]) & pos.bitboards[OCCUPANCY];
    U64 xray = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY] & ~blockers);
    U64 pinners = xray & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);

    U64 pin_rays = bishopAttacksFromBitboard(pinners, pos.bitboards[OCCUPANCY] & ~blockers) & xray;

    U64 pinned_pieces = pin_rays & pos.bitboards[friendly_occupancy];

    // moves for pinned pawns on diagonals

    // promotions first
    U64 pinned_pawns;

    if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
        if constexpr(side == WHITE) {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & RANK_7;
        } else {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & RANK_2;
        }

        while (pinned_pawns) {
            // promotion captures
            int piece_index = popLSB(pinned_pawns);
            U64 moves_bb = pawn_attacks[side][piece_index] & pinners & capture_mask;
            addMovesFromBitboard(piece_index, moves_bb, QUEEN_PROMOTION_CAPTURE, moves);
            addMovesFromBitboard(piece_index, moves_bb, ROOK_PROMOTION_CAPTURE, moves);
            addMovesFromBitboard(piece_index, moves_bb, BISHOP_PROMOTION_CAPTURE, moves);
            addMovesFromBitboard(piece_index, moves_bb, KNIGHT_PROMOTION_CAPTURE, moves);
        }

        if constexpr(side == WHITE) {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & ~RANK_7;
        } else {
            pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & ~RANK_2;
        }

        while (pinned_pawns) {
            // normal captures
            int piece_index = popLSB(pinned_pawns);
            addMovesFromBitboard(piece_index, pawn_attacks[side][piece_index] & pinners & capture_mask, CAPTURE_MOVE, moves);
            // en passant
            if (pos.en_passant) {
                addMovesFromBitboard(piece_index, pawn_attacks[side][piece_index] & 
                    (1ULL << pos.en_passant) & pin_rays & capture_mask, EN_PASSANT_CAPTURE, moves);
            }
        }
    }

    // moves for bishops and queens pinned on diagonals

    U64 pinned_bishops_and_queens = pinned_pieces & (pos.bitboards[friendly_bishop] | pos.bitboards[friendly_queen]);
    U64 magic_attack;

    while (pinned_bishops_and_queens) {
        int piece_index = popLSB(pinned_bishops_and_queens);
        magic_attack = getMagicBishopAttack(piece_index, pos.bitboards[OCCUPANCY]);
        // quiet moves
        if constexpr(gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pin_rays & block_mask, QUIET_MOVE, moves);
        }
        // captures
        if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pinners & capture_mask, CAPTURE_MOVE, moves);
        }
    }

    // pinned on ranks and files

    blockers = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]) & pos.bitboards[OCCUPANCY];
    xray = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY] & ~blockers);
    pinners = xray & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);

    pin_rays = rookAttacksFromBitboard(pinners, pos.bitboards[OCCUPANCY] & ~blockers) & xray;

    U64 pinned_on_rank_and_file = pin_rays & pos.bitboards[friendly_occupancy];
    pinned_pieces |= pinned_on_rank_and_file;


    // pinned pawns on files
    if constexpr(gen_type == QUIET || gen_type == LEGAL) {
        pinned_pawns = pinned_on_rank_and_file & pos.bitboards[friendly_pawn];

        while(pinned_pawns) {
            int piece_index = popLSB(pinned_pawns);
            addMovesFromBitboard(piece_index, pawn_pushes[side][piece_index] & pin_rays & block_mask, QUIET_MOVE, moves);
            if constexpr(side == WHITE) {
                addMovesFromBitboard(piece_index, pawn_double_pushes[side][piece_index] & RANK_4 & 
                    pin_rays & block_mask & ~pos.bitboards[OCCUPANCY] << 8, DOUBLE_PAWN_PUSH, moves);
            } else {
                addMovesFromBitboard(piece_index, pawn_double_pushes[side][piece_index] & RANK_5 & 
                    pin_rays & block_mask & ~pos.bitboards[OCCUPANCY] >> 8, DOUBLE_PAWN_PUSH, moves);
            }
        }
    }

    // pinned rooks and queens
    U64 pinned_rooks_and_queens = pinned_on_rank_and_file & (pos.bitboards[friendly_rook] | pos.bitboards[friendly_queen]);

    while (pinned_rooks_and_queens) {
        int piece_index = popLSB(pinned_rooks_and_queens);
        magic_attack = getMagicRookAttack(piece_index, pos.bitboards[OCCUPANCY]);
        // quiet moves
        if constexpr(gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & block_mask & pin_rays, QUIET_MOVE, moves);
        }
        // captures
        if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pinners & capture_mask, CAPTURE_MOVE, moves);
        }
    }

    // knight moves
    U64 knights = pos.bitboards[friendly_knight] & ~pinned_pieces;

    while(knights) {
        int piece_index = popLSB(knights);
        if constexpr(gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, knight_moves[piece_index] & ~pos.bitboards[OCCUPANCY] & block_mask, QUIET_MOVE, moves);
        }
        if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
        addMovesFromBitboard(piece_index, knight_moves[piece_index] & pos.bitboards[enemy_occupancy] & 
            capture_mask, CAPTURE_MOVE, moves);
        }
    }

    // pawn moves
    U64 pawns = pos.bitboards[friendly_pawn] & ~pinned_pieces;

    U64 double_pushes;
    U64 single_pushes;
    U64 left_attacks;
    U64 right_attacks;
    if constexpr(side == WHITE) {
        if constexpr(gen_type == QUIET || gen_type == LEGAL) {
            double_pushes = (pawns & RANK_2) << 16 & block_mask & ~pos.bitboards[OCCUPANCY] & ~pos.bitboards[OCCUPANCY] << 8;
            single_pushes = pawns << 8 & block_mask & ~pos.bitboards[OCCUPANCY];
        }
        if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
            left_attacks = pawns << 7 & pos.bitboards[enemy_occupancy] & capture_mask & ~H_FILE;
            right_attacks = pawns << 9 & pos.bitboards[enemy_occupancy] & capture_mask & ~A_FILE;
        }
    } else {
        if constexpr(gen_type == QUIET || gen_type == LEGAL) {
            double_pushes = (pawns & RANK_7) >> 16 & block_mask & ~pos.bitboards[OCCUPANCY] & ~pos.bitboards[OCCUPANCY] >> 8;
            single_pushes = pawns >> 8 & block_mask & ~pos.bitboards[OCCUPANCY];
        }
        if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
            left_attacks = pawns >> 9 & pos.bitboards[enemy_occupancy] & capture_mask & ~H_FILE;
            right_attacks = pawns >> 7 & pos.bitboards[enemy_occupancy] & capture_mask & ~A_FILE;
        }
    }


    U64 promotions;
    U64 left_promotion_captures;
    U64 right_promotion_captures;

    if constexpr(gen_type == QUIET || gen_type == LEGAL) {
        promotions = single_pushes & (RANK_8 | RANK_1);
        single_pushes = single_pushes & ~promotions;
    }
    if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
        left_promotion_captures = left_attacks & (RANK_8 | RANK_1);
        left_attacks = left_attacks & ~left_promotion_captures;
        right_promotion_captures = right_attacks & (RANK_8 | RANK_1);
        right_attacks = right_attacks & ~right_promotion_captures;
    }

    int start_square;
    int end_square;


    if constexpr(gen_type == QUIET || gen_type == LEGAL) {
        while (promotions) {
            end_square = popLSB(promotions);
            if constexpr(side == WHITE) {
                start_square = end_square - 8;
            } else {
                start_square = end_square + 8;
            }
            moves.addMove(encodeMove(start_square, end_square, QUEEN_PROMOTION));
            moves.addMove(encodeMove(start_square, end_square, KNIGHT_PROMOTION));
            moves.addMove(encodeMove(start_square, end_square, BISHOP_PROMOTION));
            moves.addMove(encodeMove(start_square, end_square, ROOK_PROMOTION));
        }

        while (double_pushes) {
            end_square = popLSB(double_pushes);
            if constexpr(side == WHITE) {
                start_square = end_square - 16;
            } else {
                start_square = end_square + 16;
            }
            moves.addMove(encodeMove(start_square, end_square, DOUBLE_PAWN_PUSH));
        }

        while (single_pushes) {
            end_square = popLSB(single_pushes);
            if constexpr(side == WHITE) {
                start_square = end_square - 8;
            } else {
                start_square = end_square + 8;
            }
            moves.addMove(encodeMove(start_square, end_square, QUIET_MOVE));
        }
    }

    if constexpr(gen_type == CAPTURES || gen_type == LEGAL) {
        while (left_promotion_captures) {
            end_square = popLSB(left_promotion_captures);
            if constexpr(side == WHITE) {
                start_square = end_square - 7;
            } else {
                start_square = end_square + 9;
            }
            moves.addMove(encodeMove(start_square, end_square, QUEEN_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, KNIGHT_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, BISHOP_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, ROOK_PROMOTION_CAPTURE));
        }

        while (right_promotion_captures) {
            end_square = popLSB(right_promotion_captures);
            if constexpr(side == WHITE) {
                start_square = end_square - 9;
            } else {
                start_square = end_square + 7;
            }
            moves.addMove(encodeMove(start_square, end_square, QUEEN_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, KNIGHT_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, BISHOP_PROMOTION_CAPTURE));
            moves.addMove(encodeMove(start_square, end_square, ROOK_PROMOTION_CAPTURE));
        }

        while (left_attacks) {
            end_square = popLSB(left_attacks);
            if constexpr(side == WHITE) {
                start_square = end_square - 7;
            } else {
                start_square = end_square + 9;
            }
            moves.addMove(encodeMove(start_square, end_square, CAPTURE_MOVE));
        }

        while (right_attacks) {
            end_square = popLSB(right_attacks);
            if constexpr(side == WHITE) {
                start_square = end_square - 9;
            } else {
                start_square = end_square + 7;
            }
            moves.addMove(encodeMove(start_square, end_square, CAPTURE_MOVE));
        }

         // en passant 
        if (pos.en_passant) {
            U64 en_passant_attackers;
            int piece_index;
            en_passant_attackers = pawn_attacks[enemy_side][pos.en_passant] & pawns;
            if constexpr(side == WHITE) {
                piece_index = pos.en_passant - 8;
            } else {
                piece_index = pos.en_passant + 8;
            }
            if ((num_checks == 0) || (pawn_attacks[enemy_side][piece_index] & pos.bitboards[friendly_king])) {
                while (en_passant_attackers) {
                    start_square = popLSB(en_passant_attackers);
                    if (pos.bitboards[friendly_king] & (RANK_5 >> (8 * side))) { 
                        // check to see if the move leaves the king in check
                        U64 ep_pin_rays = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY] & ~setBit(start_square) & ~setBit(piece_index));
                        if (ep_pin_rays & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen])) {
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

    U64 bishops = (pos.bitboards[friendly_bishop] | pos.bitboards[friendly_queen]) & ~pinned_pieces;

    while(bishops) {
        int piece_index = popLSB(bishops);
        magic_attack = getMagicBishopAttack(piece_index, pos.bitboards[OCCUPANCY]);
        if constexpr(gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & ~pos.bitboards[OCCUPANCY] & block_mask, QUIET_MOVE, moves);
        }
        if constexpr(gen_type == CAPTURES|| gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pos.bitboards[enemy_occupancy] & capture_mask, CAPTURE_MOVE, moves);
        }
    }

    // rook moves (including queens)

    U64 rooks = (pos.bitboards[friendly_rook] | pos.bitboards[friendly_queen]) & ~pinned_pieces;

    while(rooks) {
        int piece_index = popLSB(rooks);
        magic_attack = getMagicRookAttack(piece_index, pos.bitboards[OCCUPANCY]);
        if constexpr(gen_type == QUIET || gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & ~pos.bitboards[OCCUPANCY] & block_mask, QUIET_MOVE, moves);
        }
        if constexpr(gen_type == CAPTURES|| gen_type == LEGAL) {
            addMovesFromBitboard(piece_index, magic_attack & pos.bitboards[enemy_occupancy] & capture_mask, CAPTURE_MOVE, moves);
        }
    }

    if constexpr(gen_type == QUIET || gen_type == LEGAL) {
        // castling
        if (num_checks == 0) {
            if constexpr(side == WHITE) {
                if ((WKC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & WKC_SQUARES) && !(enemy_attacks & WKC_KING_SQUARES)) {
                    moves.addMove(encodeMove(e1, g1, KING_CASTLE));
                }
                if ((WQC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & WQC_SQUARES) && !(enemy_attacks & WQC_KING_SQUARES)) {
                    moves.addMove(encodeMove(e1, c1, QUEEN_CASTLE));
                }
            } else {
                if ((BKC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & BKC_SQUARES) && !(enemy_attacks & BKC_KING_SQUARES)) {
                    moves.addMove(encodeMove(e8, g8, KING_CASTLE));
                }
                if ((BQC & pos.castle_rights) && !(pos.bitboards[OCCUPANCY] & BQC_SQUARES) && !(enemy_attacks & BQC_KING_SQUARES)) {
                    moves.addMove(encodeMove(e8, c8, QUEEN_CASTLE));
                }
            }
        }
    }
}

template <Color side>
bool inCheckSided(Position &pos) {

    if (pos.in_check) {
        return true;
    }

    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const int enemy_side = getOtherSide(side);

    constexpr const int friendly_pawn = getPieceID(PAWN, side);
    constexpr const int friendly_knight = getPieceID(KNIGHT, side);
    constexpr const int friendly_bishop = getPieceID(BISHOP, side);
    constexpr const int friendly_rook = getPieceID(ROOK, side);
    constexpr const int friendly_queen = getPieceID(QUEEN, side);
    constexpr const int friendly_king = getPieceID(KING, side);

    constexpr const int enemy_pawn = getPieceID(PAWN, enemy_side);
    constexpr const int enemy_knight = getPieceID(KNIGHT, enemy_side);
    constexpr const int enemy_bishop = getPieceID(BISHOP, enemy_side);
    constexpr const int enemy_rook = getPieceID(ROOK, enemy_side);
    constexpr const int enemy_queen = getPieceID(QUEEN, enemy_side);
    constexpr const int enemy_king = getPieceID(KING, enemy_side);

    constexpr const int friendly_occupancy = getOccupancy(side);
    constexpr const int enemy_occupancy = getOccupancy(enemy_side);

    // find the checks on the king and create attack and block masks

    U64 checkers = 0ULL;
    int king_index = bitScanForward(pos.bitboards[friendly_king]);

    checkers |= knight_moves[king_index] & pos.bitboards[enemy_knight];
    checkers |= pawn_attacks[side][king_index] & pos.bitboards[enemy_pawn];
    U64 bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[OCCUPANCY]);
    U64 checking_bishops = bishop_rays_from_king & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
    checkers |= checking_bishops;
    U64 rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[OCCUPANCY]);
    U64 checking_rooks = rook_rays_from_king & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);
    checkers |= checking_rooks;
    // king check detection needed for pseudolegal moves
    checkers |= king_moves[king_index] & pos.bitboards[enemy_king];
    U64 block_mask = bishopAttacksFromBitboard(checking_bishops, pos.bitboards[OCCUPANCY]) & bishop_rays_from_king;
    block_mask |= rookAttacksFromBitboard(checking_rooks, pos.bitboards[OCCUPANCY]) & rook_rays_from_king;

    if (checkers) {
        pos.in_check = true;
        return true;
    }
    return false;
}

bool inCheck(Position &pos);

bool sideToPlayInCheck(Position &pos);

U64 perftHelper(Position &pos, int depth);

U64 perft(Position &pos, int depth);

U64 playablePerftHelper(Position &pos, int depth);

// perft function for testing the isPlayable function
U64 playablePerft(Position &pos, int depth);
