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

    // pos.readFen("8/8/6p1/5p1p/2p2r2/1k6/1P1R1B2/1K6 b - - 5 53");

    // std::cout << eval(pos) << std::endl;

    // generateMoves<false>(moves, pos);

    // std::cout << moves.size() << std::endl;

    // for (const auto &m: moves) {
    //     std::cout << moveToString(m) << std::endl;
    // }

    // Search search;

    // move16 best_move = search.iterSearch(pos, 15, 4000ULL);

    // pos.print();
    // pos.printFromBitboard();

    // printMoveLong(best_move);

    // std::cout << perft(pos, 7) << std::endl;

}