#pragma once

#include <iostream>

#include "types.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "bitboards.hpp"
#include "move.hpp"


template <bool white_to_move>
void generateMoves(MoveList &moves, Position &pos) {

    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const int side = static_cast<int>(white_to_move) ^ 1;
    constexpr const int enemy_side = side ^ 1;

    constexpr const int friendly_pawn = getPieceID(white_pawn, side);
    constexpr const int friendly_knight = getPieceID(white_knight, side);
    constexpr const int friendly_bishop = getPieceID(white_bishop, side);
    constexpr const int friendly_rook = getPieceID(white_rook, side);
    constexpr const int friendly_queen = getPieceID(white_queen, side);
    constexpr const int friendly_king = getPieceID(white_king, side);

    constexpr const int enemy_pawn = getPieceID(white_pawn, enemy_side);
    constexpr const int enemy_knight = getPieceID(white_knight, enemy_side);
    constexpr const int enemy_bishop = getPieceID(white_bishop, enemy_side);
    constexpr const int enemy_rook = getPieceID(white_rook, enemy_side);
    constexpr const int enemy_queen = getPieceID(white_queen, enemy_side);
    constexpr const int enemy_king = getPieceID(white_king, enemy_side);

    constexpr const int friendly_occupancy = getOccupancy(side);
    constexpr const int enemy_occupancy = getOccupancy(enemy_side);


    // find all squares attacked by the opponent 

    U64 enemy_attacks = pawnAttacksFromBitboard<!white_to_move>(pos.bitboards[enemy_pawn]);
    enemy_attacks |= knightAttacksFromBitboard(pos.bitboards[enemy_knight]);
    // treat the king as invisible for bishops and rooks
    enemy_attacks |= bishopAttacksFromBitboard(pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen], pos.bitboards[occupancy] & ~pos.bitboards[friendly_king]);
    enemy_attacks |= rookAttacksFromBitboard(pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen], pos.bitboards[occupancy] & ~pos.bitboards[friendly_king]);
    enemy_attacks |= king_moves[bitScanForward(pos.bitboards[enemy_king])];

    // find the checks on the king and create attack and block masks

    U64 checkers = 0ULL;
    int king_index = bitScanForward(pos.bitboards[friendly_king]);

    checkers |= knight_moves[king_index] & pos.bitboards[enemy_knight];
    checkers |= pawn_attacks[side][king_index] & pos.bitboards[enemy_pawn];
    U64 bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[occupancy]);
    U64 checking_bishops = bishop_rays_from_king & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
    checkers |= checking_bishops;
    U64 rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[occupancy]);
    U64 checking_rooks = rook_rays_from_king & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);
    checkers |= checking_rooks;
    U64 block_mask = bishopAttacksFromBitboard(checking_bishops, pos.bitboards[occupancy]) & bishop_rays_from_king;
    block_mask |= rookAttacksFromBitboard(checking_rooks, pos.bitboards[occupancy]) & rook_rays_from_king;

    int num_checks = countBits(checkers);

    if (num_checks > 0) {
        pos.in_check = true;
    } else {
        pos.in_check = false;
    }

    // generate king moves

    // add quiet moves
    addMovesFromBitboard(king_index, king_moves[king_index] & ~enemy_attacks & ~pos.bitboards[occupancy], quiet_move, moves);
    // add captures
    addMovesFromBitboard(king_index, king_moves[king_index] & ~enemy_attacks & pos.bitboards[enemy_occupancy], capture_move, moves);

    if (num_checks >= 2) {
        return;
    }

    U64 capture_mask = checkers;

    if (num_checks == 0) {
        block_mask = ~0ULL;
        capture_mask = ~0ULL;
    }

    // generate moves for pinned pieces

    // pinned on diagonals
    U64 blockers = getMagicBishopAttack(king_index, pos.bitboards[occupancy]) & pos.bitboards[occupancy];
    U64 xray = getMagicBishopAttack(king_index, pos.bitboards[occupancy] & ~blockers);
    U64 pinners = xray & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);

    U64 pin_rays = bishopAttacksFromBitboard(pinners, pos.bitboards[occupancy] & ~blockers) & xray;

    U64 pinned_pieces = pin_rays & pos.bitboards[friendly_occupancy];

    // moves for pinned pawns on diagonals

    // promotions first
    U64 pinned_pawns;
    if constexpr(white_to_move) {
        pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & rank_7;
    } else {
        pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & rank_2;
    }

    int piece_index;

    U64 moves_bb;
    while (pinned_pawns) {
        // promotion captures
        piece_index = popLSB(pinned_pawns);
        moves_bb = pawn_attacks[side][piece_index] & pinners & capture_mask;
        addMovesFromBitboard(piece_index, moves_bb, queen_promotion_capture, moves);
        addMovesFromBitboard(piece_index, moves_bb, rook_promotion_capture, moves);
        addMovesFromBitboard(piece_index, moves_bb, bishop_promotion_capture, moves);
        addMovesFromBitboard(piece_index, moves_bb, knight_promotion_capture, moves);
    }

    if constexpr(white_to_move) {
        pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & ~rank_7;
    } else {
        pinned_pawns = pinned_pieces & pos.bitboards[friendly_pawn] & ~rank_2;
    }
    while (pinned_pawns) {
        // normal captures
        piece_index = popLSB(pinned_pawns);
        addMovesFromBitboard(piece_index, pawn_attacks[side][piece_index] & pinners & capture_mask, capture_move, moves);
        // en passant
        if (pos.en_passant) {
            addMovesFromBitboard(piece_index, pawn_attacks[side][piece_index] & (1ULL << pos.en_passant) & pin_rays & capture_mask, en_passant_capture, moves);
        }
    }

    // moves for bishops and queens pinned on diagonals

    U64 pinned_bishops_and_queens = pinned_pieces & (pos.bitboards[friendly_bishop] | pos.bitboards[friendly_queen]);
    U64 magic_attack;

    while (pinned_bishops_and_queens) {
        piece_index = popLSB(pinned_bishops_and_queens);
        magic_attack = getMagicBishopAttack(piece_index, pos.bitboards[occupancy]);
        // quiet moves
        addMovesFromBitboard(piece_index, magic_attack & pin_rays & block_mask, quiet_move, moves);
        // captures
        addMovesFromBitboard(piece_index, magic_attack & pinners & capture_mask, capture_move, moves);
    }

    // pinned on ranks and files

    blockers = getMagicRookAttack(king_index, pos.bitboards[occupancy]) & pos.bitboards[occupancy];
    xray = getMagicRookAttack(king_index, pos.bitboards[occupancy] & ~blockers);
    pinners = xray & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);

    pin_rays = rookAttacksFromBitboard(pinners, pos.bitboards[occupancy] & ~blockers) & xray;

    U64 pinned_on_rank_and_file = pin_rays & pos.bitboards[friendly_occupancy];
    pinned_pieces |= pinned_on_rank_and_file;


    // pinned pawns on files
    pinned_pawns = pinned_on_rank_and_file & pos.bitboards[friendly_pawn];

    while(pinned_pawns) {
        piece_index = popLSB(pinned_pawns);
        addMovesFromBitboard(piece_index, pawn_pushes[side][piece_index] & pin_rays & block_mask, quiet_move, moves);
        if constexpr(white_to_move) {
            addMovesFromBitboard(piece_index, pawn_double_pushes[side][piece_index] & rank_4 & pin_rays & block_mask & ~pos.bitboards[occupancy] << 8, double_pawn_push, moves);
        } else {
            addMovesFromBitboard(piece_index, pawn_double_pushes[side][piece_index] & rank_5 & pin_rays & block_mask & ~pos.bitboards[occupancy] >> 8, double_pawn_push, moves);
        }
    }

    // pinned rooks and queens

    U64 pinned_rooks_and_queens = pinned_on_rank_and_file & (pos.bitboards[friendly_rook] | pos.bitboards[friendly_queen]);

    while (pinned_rooks_and_queens) {
        piece_index = popLSB(pinned_rooks_and_queens);
        magic_attack = getMagicRookAttack(piece_index, pos.bitboards[occupancy]);
        // quiet moves
        addMovesFromBitboard(piece_index, magic_attack & block_mask & pin_rays, quiet_move, moves);
        // captures
        addMovesFromBitboard(piece_index, magic_attack & pinners & capture_mask, capture_move, moves);
    }

    // knight moves
    U64 knights = pos.bitboards[friendly_knight] & ~pinned_pieces;

    while(knights) {
        piece_index = popLSB(knights);
        addMovesFromBitboard(piece_index, knight_moves[piece_index] & ~pos.bitboards[occupancy] & block_mask, quiet_move, moves);
        addMovesFromBitboard(piece_index, knight_moves[piece_index] & pos.bitboards[enemy_occupancy] & capture_mask, capture_move, moves);
    }

    // pawn moves

    U64 pawns = pos.bitboards[friendly_pawn] & ~pinned_pieces;

    // single and double pushes

    U64 double_pushes;
    U64 single_pushes;
    U64 left_attacks;
    U64 right_attacks;
    if constexpr(white_to_move) {
        double_pushes = (pawns & rank_2) << 16 & block_mask & ~pos.bitboards[occupancy] & ~pos.bitboards[occupancy] << 8;
        single_pushes = pawns << 8 & block_mask & ~pos.bitboards[occupancy];
        left_attacks = pawns << 7 & pos.bitboards[enemy_occupancy] & capture_mask & ~h_file;
        right_attacks = pawns << 9 & pos.bitboards[enemy_occupancy] & capture_mask & ~a_file;
    } else {
        double_pushes = (pawns & rank_7) >> 16 & block_mask & ~pos.bitboards[occupancy] & ~pos.bitboards[occupancy] >> 8;
        single_pushes = pawns >> 8 & block_mask & ~pos.bitboards[occupancy];
        left_attacks = pawns >> 9 & pos.bitboards[enemy_occupancy] & capture_mask & ~h_file;
        right_attacks = pawns >> 7 & pos.bitboards[enemy_occupancy] & capture_mask & ~a_file;
    }

    U64 promotions = single_pushes & (rank_8 | rank_1);
    single_pushes = single_pushes & ~promotions;
    U64 left_promotion_captures = left_attacks & (rank_8 | rank_1);
    left_attacks = left_attacks & ~left_promotion_captures;
    U64 right_promotion_captures = right_attacks & (rank_8 | rank_1);
    right_attacks = right_attacks & ~right_promotion_captures;

    int start_square;
    int end_square;

    while (promotions) {
        end_square = popLSB(promotions);
        if constexpr(white_to_move) {
            start_square = end_square - 8;
        } else {
            start_square = end_square + 8;
        }
        moves.addMove(encodeMove(start_square, end_square, queen_promotion));
        moves.addMove(encodeMove(start_square, end_square, knight_promotion));
        moves.addMove(encodeMove(start_square, end_square, bishop_promotion));
        moves.addMove(encodeMove(start_square, end_square, rook_promotion));
    }

    while (left_promotion_captures) {
        end_square = popLSB(left_promotion_captures);
        if constexpr(white_to_move) {
            start_square = end_square - 7;
        } else {
            start_square = end_square + 9;
        }
        moves.addMove(encodeMove(start_square, end_square, queen_promotion_capture));
        moves.addMove(encodeMove(start_square, end_square, knight_promotion_capture));
        moves.addMove(encodeMove(start_square, end_square, bishop_promotion_capture));
        moves.addMove(encodeMove(start_square, end_square, rook_promotion_capture));
    }

    while (right_promotion_captures) {
        end_square = popLSB(right_promotion_captures);
        if constexpr(white_to_move) {
            start_square = end_square - 9;
        } else {
            start_square = end_square + 7;
        }
        moves.addMove(encodeMove(start_square, end_square, queen_promotion_capture));
        moves.addMove(encodeMove(start_square, end_square, knight_promotion_capture));
        moves.addMove(encodeMove(start_square, end_square, bishop_promotion_capture));
        moves.addMove(encodeMove(start_square, end_square, rook_promotion_capture));
    }


    while (double_pushes) {
        end_square = popLSB(double_pushes);
        if constexpr(white_to_move) {
            start_square = end_square - 16;
        } else {
            start_square = end_square + 16;
        }
        moves.addMove(encodeMove(start_square, end_square, double_pawn_push));
    }

    while (single_pushes) {
        end_square = popLSB(single_pushes);
        if constexpr(white_to_move) {
            start_square = end_square - 8;
        } else {
            start_square = end_square + 8;
        }
        moves.addMove(encodeMove(start_square, end_square, quiet_move));
    }

    while (left_attacks) {
        end_square = popLSB(left_attacks);
        if constexpr(white_to_move) {
            start_square = end_square - 7;
        } else {
            start_square = end_square + 9;
        }
        moves.addMove(encodeMove(start_square, end_square, capture_move));
    }

    while (right_attacks) {
        end_square = popLSB(right_attacks);
        if constexpr(white_to_move) {
            start_square = end_square - 9;
        } else {
            start_square = end_square + 7;
        }
        moves.addMove(encodeMove(start_square, end_square, capture_move));
    }

    // en passant 
    if (pos.en_passant) {
        U64 en_passant_attackers;
        pin_rays = 0ULL;
        en_passant_attackers = pawn_attacks[enemy_side][pos.en_passant] & pawns;
        if constexpr(white_to_move) {
            piece_index = pos.en_passant - 8;
        } else {
            piece_index = pos.en_passant + 8;
        }
        if ((num_checks == 0) || (pawn_attacks[enemy_side][piece_index] & pos.bitboards[friendly_king])) {
            while (en_passant_attackers) {
                start_square = popLSB(en_passant_attackers);
                if (pos.bitboards[friendly_king] & (rank_5 >> (8 * side))) { 
                    pin_rays = getMagicRookAttack(king_index, pos.bitboards[occupancy] & ~setBit(start_square) & ~setBit(piece_index));
                    if (pin_rays & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen])) {
                        break;
                    }
                }
                moves.addMove(encodeMove(start_square, pos.en_passant, en_passant_capture));
            }
        }
    }


    // sliding piece moves

    // bishop moves (including queens)

    U64 bishops = (pos.bitboards[friendly_bishop] | pos.bitboards[friendly_queen]) & ~pinned_pieces;

    while(bishops) {
        piece_index = popLSB(bishops);
        magic_attack = getMagicBishopAttack(piece_index, pos.bitboards[occupancy]);
        addMovesFromBitboard(piece_index, magic_attack & ~pos.bitboards[occupancy] & block_mask, quiet_move, moves);
        addMovesFromBitboard(piece_index, magic_attack & pos.bitboards[enemy_occupancy] & capture_mask, capture_move, moves);
    }

    // rook moves (including queens)

    U64 rooks = (pos.bitboards[friendly_rook] | pos.bitboards[friendly_queen]) & ~pinned_pieces;

    // printBitboard(capture_mask);
    // printBitboard(pos.bitboards[white_pawn]);

    while(rooks) {
        piece_index = popLSB(rooks);
        magic_attack = getMagicRookAttack(piece_index, pos.bitboards[occupancy]);
        addMovesFromBitboard(piece_index, magic_attack & ~pos.bitboards[occupancy] & block_mask, quiet_move, moves);
        addMovesFromBitboard(piece_index, magic_attack & pos.bitboards[enemy_occupancy] & capture_mask, capture_move, moves);
    }

    // castling
    if (num_checks == 0) {
        if constexpr(white_to_move) {
            if ((wkc & pos.castle_rights) && !(pos.bitboards[occupancy] & wkc_squares) && !(enemy_attacks & wkc_king_squares)) {
                moves.addMove(encodeMove(e1, g1, king_castle));
            }
            if ((wqc & pos.castle_rights) && !(pos.bitboards[occupancy] & wqc_squares) && !(enemy_attacks & wqc_king_squares)) {
                moves.addMove(encodeMove(e1, c1, queen_castle));
            }
        } else {
            if ((bkc & pos.castle_rights) && !(pos.bitboards[occupancy] & bkc_squares) && !(enemy_attacks & bkc_king_squares)) {
                moves.addMove(encodeMove(e8, g8, king_castle));
            }
            if ((bqc & pos.castle_rights) && !(pos.bitboards[occupancy] & bqc_squares) && !(enemy_attacks & bqc_king_squares)) {
                moves.addMove(encodeMove(e8, c8, queen_castle));
            }
        }
    }
}

template <bool white_to_move>
void inCheck(Position &pos) {

    // set up all bitboards for easy access according to friendly vs enemy with compiletime stuff
    constexpr const int side = static_cast<int>(white_to_move) ^ 1;
    constexpr const int enemy_side = side ^ 1;

    constexpr const int friendly_pawn = getPieceID(white_pawn, side);
    constexpr const int friendly_knight = getPieceID(white_knight, side);
    constexpr const int friendly_bishop = getPieceID(white_bishop, side);
    constexpr const int friendly_rook = getPieceID(white_rook, side);
    constexpr const int friendly_queen = getPieceID(white_queen, side);
    constexpr const int friendly_king = getPieceID(white_king, side);

    constexpr const int enemy_pawn = getPieceID(white_pawn, enemy_side);
    constexpr const int enemy_knight = getPieceID(white_knight, enemy_side);
    constexpr const int enemy_bishop = getPieceID(white_bishop, enemy_side);
    constexpr const int enemy_rook = getPieceID(white_rook, enemy_side);
    constexpr const int enemy_queen = getPieceID(white_queen, enemy_side);
    constexpr const int enemy_king = getPieceID(white_king, enemy_side);

    constexpr const int friendly_occupancy = getOccupancy(side);
    constexpr const int enemy_occupancy = getOccupancy(enemy_side);


    // find all squares attacked by the opponent 

    U64 enemy_attacks = pawnAttacksFromBitboard<!white_to_move>(pos.bitboards[enemy_pawn]);
    enemy_attacks |= knightAttacksFromBitboard(pos.bitboards[enemy_knight]);
    // treat the king as invisible for bishops and rooks
    enemy_attacks |= bishopAttacksFromBitboard(pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen], pos.bitboards[occupancy] & ~pos.bitboards[friendly_king]);
    enemy_attacks |= rookAttacksFromBitboard(pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen], pos.bitboards[occupancy] & ~pos.bitboards[friendly_king]);
    enemy_attacks |= king_moves[bitScanForward(pos.bitboards[enemy_king])];

    // find the checks on the king and create attack and block masks

    U64 checkers = 0ULL;
    int king_index = bitScanForward(pos.bitboards[friendly_king]);

    checkers |= knight_moves[king_index] & pos.bitboards[enemy_knight];
    checkers |= pawn_attacks[side][king_index] & pos.bitboards[enemy_pawn];
    U64 bishop_rays_from_king = getMagicBishopAttack(king_index, pos.bitboards[occupancy]);
    U64 checking_bishops = bishop_rays_from_king & (pos.bitboards[enemy_bishop] | pos.bitboards[enemy_queen]);
    checkers |= checking_bishops;
    U64 rook_rays_from_king = getMagicRookAttack(king_index, pos.bitboards[occupancy]);
    U64 checking_rooks = rook_rays_from_king & (pos.bitboards[enemy_rook] | pos.bitboards[enemy_queen]);
    checkers |= checking_rooks;
    U64 block_mask = bishopAttacksFromBitboard(checking_bishops, pos.bitboards[occupancy]) & bishop_rays_from_king;
    block_mask |= rookAttacksFromBitboard(checking_rooks, pos.bitboards[occupancy]) & rook_rays_from_king;

    if (checkers) {
        return true;
    }
    return false;
}

U64 perftHelper(Position &pos, int depth);

U64 perft(Position &pos, int depth);