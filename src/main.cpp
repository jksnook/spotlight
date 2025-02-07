#include <iostream>
#include <string>

#include "bitboards.hpp"
#include "zobrist.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "uci.hpp"
#include "eval.hpp"
#include "search.hpp"

int main() {
    initMoves();
    initMagics();
    initZobrist();

    UCI uci;

    uci.loop();

    // Position pos;

    // std::cout << eval(pos) << std::endl;

    // MoveList moves;

    // pos.readFen("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -");

    // // std::cout << eval(pos) << std::endl;

    // generateMoves<true>(moves, pos);

    // std::cout << moves.size() << std::endl;

    // for (const auto &m: moves) {
    //     std::cout << moveToString(m) << std::endl;
    // }

    // int e = see(pos, moves[7]);

    // std::cout << "static exchange: " << e << std::endl;

    // Search search;

    // move16 best_move = search.iterSearch(pos, 15, 4000ULL);

    // pos.print();
    // pos.printFromBitboard();

    // printMoveLong(best_move);

    // std::cout << perft(pos, 7) << std::endl;

    // problem with fen r1bqkbnr/p1p5/2n3pp/1pP1pp2/1P1pPP2/B2P4/P2KN1PP/RN1Q1B1R b kq - 1 9

}