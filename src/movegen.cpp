#include "movegen.hpp"

void generateCaptures(MoveList &moves, Position &pos) {
    if (pos.side_to_move == WHITE) {
        generateCapturesSided<true>(moves, pos);
    } else {
        generateCapturesSided<false>(moves, pos);
    }
}

void generateQuietMoves(MoveList &moves, Position &pos) {
    if (pos.side_to_move == WHITE) {
        generateQuietMovesSided<true>(moves, pos);
    } else {
        generateQuietMovesSided<false>(moves, pos);
    }
}

//  Checks if a move can be played without breaking makemove or search
bool isPlayable(move16 move, Position &pos) {
    int move_type = getMoveType(move);
    int from_sq = getFromSquare(move);
    int to_sq = getToSquare(move);

    if (!(pos.bitboards[WHITE_OCCUPANCY + pos.side_to_move] & setBit(from_sq))) {
        return false;
    }
    
    if (move_type & PROMOTION_FLAG && pos.at(from_sq) != PAWN + pos.side_to_move * 6) {
        return false;
    }

    if (move_type == EN_PASSANT_CAPTURE) {
        if (pos.at(to_sq + 8 * pos.side_to_move - 8 * (1 - pos.side_to_move)) != BLACK_PAWN - pos.side_to_move * 6) {
            return false;
        }   
    } else if (move_type & CAPTURE_MOVE) {
        if (!(pos.bitboards[WHITE_OCCUPANCY + (pos.side_to_move ^ 1)] & setBit(to_sq))) {
            return false;
        }
    } else {
        if (pos.bitboards[OCCUPANCY] & setBit(to_sq)) {
            return false;
        }
    }

    if (move_type == QUEEN_CASTLE) {
        if (pos.side_to_move == WHITE) {
            if ((pos.bitboards[OCCUPANCY] & WQC_KING_SQUARES) || !(setBit(A1) & pos.bitboards[WHITE_ROOK])) {
                return false;
            }
        } else {
            if ((pos.bitboards[OCCUPANCY] & BQC_KING_SQUARES) || !(setBit(A8) & pos.bitboards[BLACK_ROOK])) {
                return false;
            }
        }
    }else if (move_type == KING_CASTLE) {
        if (pos.side_to_move == WHITE) {
            if ((pos.bitboards[OCCUPANCY] & WKC_KING_SQUARES) || !(setBit(H1) & pos.bitboards[WHITE_ROOK])) {
                return false;
            }
        } else {
            if ((pos.bitboards[OCCUPANCY] & BKC_KING_SQUARES) || !(setBit(H8) & pos.bitboards[BLACK_ROOK])) {
                return false;
            }
        }
    }

    return true;
}

bool inCheck(Position &pos) {
    if (pos.side_to_move == WHITE) {
        return inCheckSided<true>(pos);
    } else {
        return inCheckSided<false>(pos);
    }
}

bool sideToPlayInCheck(Position &pos) {
    if (pos.side_to_move == BLACK) {
        return inCheckSided<true>(pos);
    } else {
        return inCheckSided<false>(pos);
    }
}

U64 perftHelper(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    if (pos.side_to_move == WHITE) {
        generateLegalMoves<true>(moves, pos);
    } else {
        generateLegalMoves<false>(moves, pos);
    }

    if (depth == 1) {
        return moves.size();
    }

    for(auto &move: moves) {
        pos.makeMove(move);
        nodes += perftHelper(pos, depth - 1);
        pos.unmakeMove();
    }

    return nodes;
};

U64 perft(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    if (pos.side_to_move == WHITE) {
        generateLegalMoves<true>(moves, pos);
    } else {
        generateLegalMoves<false>(moves, pos);
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

U64 playablePerftHelper(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    for(auto &move: moves) {
        if (!isPlayable(move, pos)) {
            std::cout << "failed before move " << moveToString(move) << "\n";
            pos.print();
        }
        assert(isPlayable(move, pos));
        pos.makeMove(move);
        if (isPlayable(move, pos)) {
            std::cout << "failed after move " << moveToString(move) << "\n";
            pos.print();
        }
        assert(!isPlayable(move, pos));
        nodes += playablePerftHelper(pos, depth - 1);
        pos.unmakeMove();
    }

    return nodes;
}

// perft function for testing the isPlayable function
U64 playablePerft(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;
    MoveList moves;

    if (pos.side_to_move == WHITE) {
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
    }

    if (depth == 1) {
        return moves.size();
    }

    for(auto &move: moves) {
        if (!isPlayable(move, pos)) {
            std::cout << "failed before move " << moveToString(move) << "\n";
            pos.print();
        }
        assert(isPlayable(move, pos));
        pos.makeMove(move);
        assert(!sideToPlayInCheck(pos));
        if (isPlayable(move, pos)) {
            std::cout << "failed after move " << moveToString(move) << "\n";
            pos.print();
        }
        assert(!isPlayable(move, pos));
        int nodes_this_move = playablePerftHelper(pos, depth - 1);
        pos.unmakeMove();
        nodes += nodes_this_move;
        std::cout << moveToString(move) << ": " << nodes_this_move << std::endl;
    }

    return nodes;
}