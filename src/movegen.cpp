#include "movegen.hpp"

U64 perftHelper(Position &pos, int depth) {
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
        generateMoves<true>(moves, pos);
    } else {
        generateMoves<false>(moves, pos);
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