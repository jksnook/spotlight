#include "movegen.hpp"
#include "moveorder.hpp"

void generateCaptures(MoveList &moves, Position &pos) {
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

U64 getEnemyAttacks(Position &pos, int sq) {
    U64 attackers = 0ULL;

    attackers |= knight_moves[sq] & (pos.bitboards[getPieceID(KNIGHT, pos.side_to_move ^ 1)]);
    attackers |= pawn_attacks[pos.side_to_move][sq] & pos.bitboards[getPieceID(PAWN, pos.side_to_move ^ 1)];
    U64 bishop_rays_from_sq = getMagicBishopAttack(sq, pos.bitboards[OCCUPANCY]);
    attackers |= bishop_rays_from_sq & (pos.bitboards[getPieceID(BISHOP, pos.side_to_move ^ 1)] | pos.bitboards[getPieceID(QUEEN, pos.side_to_move ^ 1)]);
    U64 rook_rays_from_sq = getMagicRookAttack(sq, pos.bitboards[OCCUPANCY]);
    attackers |= rook_rays_from_sq & (pos.bitboards[getPieceID(ROOK, pos.side_to_move ^ 1)] | pos.bitboards[getPieceID(QUEEN, pos.side_to_move ^ 1)]);
    attackers |= king_moves[sq] & pos.bitboards[getPieceID(KING, pos.side_to_move ^ 1)];

    return attackers;
}

bool isPseudoLegal(move16 move, Position &pos) {
    if (move == 0) {
        return false;
    }

    int from_sq = getFromSquare(move);
    int to_sq = getToSquare(move);
    int piece = pos.at(from_sq);
    int piece_type = piece % 6;

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
            if (!(setBit(to_sq) & (pawn_pushes[pos.side_to_move][from_sq]) || move_type == DOUBLE_PAWN_PUSH)) {
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
        if (!(setBit(to_sq) & (getMagicRookAttack(from_sq, pos.bitboards[OCCUPANCY]) | getMagicBishopAttack(from_sq, pos.bitboards[OCCUPANCY])))) {
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
        if (piece_type != PAWN || !(setBit(to_sq) & pawn_double_pushes[pos.side_to_move][from_sq])) {
            return false;
        }
        if (pawn_pushes[pos.side_to_move][from_sq] & pos.bitboards[OCCUPANCY]) return false;
    } else if (move_type == EN_PASSANT_CAPTURE) {
        if (piece_type != PAWN || !pos.en_passant || to_sq != pos.en_passant ) {
            return false;
        }
    } else if (move_type == KING_CASTLE || move_type == QUEEN_CASTLE) {
        if (piece_type != KING) return false;
        U64 pass_through;
        U64 rook_sq_bb;
        U64 rook_bb;
        U64 temp;
        if (move_type == KING_CASTLE) {
            if (!(pos.castle_rights & (WKC <<( 2 * pos.side_to_move)))) return false;
            if (from_sq != E1 + 56 * pos.side_to_move || to_sq != G1 + 56 * pos.side_to_move) {
                return false;
            }
            pass_through = WKC_SQUARES << 56 * pos.side_to_move;
            rook_sq_bb = setBit(H1) << 56 * pos.side_to_move;
            rook_bb = pos.bitboards[getPieceID(ROOK, pos.side_to_move)];
            temp = (setBit(E1) | setBit(F1)) << 56 * pos.side_to_move;
        } else if (move_type == QUEEN_CASTLE) {
            if (!(pos.castle_rights & (WQC <<( 2 * pos.side_to_move)))) return false;
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
            int sq = popLSB(temp);
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

U64 perftHelper(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    // generateMoves(moves, pos);

    if (pos.side_to_move == WHITE) {
        // generateMoves<WHITE, LEGAL>(moves, pos);
        generateMovesSided<WHITE, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<WHITE, QUIET>(moves, pos);
    } else {
        // generateMoves<BLACK, LEGAL>(moves, pos);
        generateMovesSided<BLACK, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<BLACK, QUIET>(moves, pos);
    }

    // if (depth == 1) {
    //     return moves.size();
    // }

    for(auto &move: moves) {
        pos.makeMove(move);
        nodes += perftHelper(pos, depth - 1);
        pos.unmakeMove();
    }

    return nodes;
};

U64 perft(Position &pos, int depth) {
    std::cout << "Bulk counting disabled\n";
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    if (pos.side_to_move == WHITE) {
        // generateMoves<WHITE, LEGAL>(moves, pos);
        generateMovesSided<WHITE, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<WHITE, QUIET>(moves, pos);
    } else {
        // generateMoves<BLACK, LEGAL>(moves, pos);
        generateMovesSided<BLACK, CAPTURES_AND_PROMOTIONS>(moves, pos);
        generateMovesSided<BLACK, QUIET>(moves, pos);
    }

    for(auto &move: moves) {
        pos.makeMove(move);
        int nodes_this_move = perftHelper(pos, depth - 1);
        pos.unmakeMove();
        nodes += nodes_this_move;
        std::cout << moveToString(move) << ": " << nodes_this_move << std::endl;
    }

    return nodes;
};