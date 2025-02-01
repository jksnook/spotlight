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

    // UCI uci;

    // uci.loop();

    Position pos;

    std::cout << eval(pos) << std::endl;

    // MoveList moves;

    pos.readFen("k7/8/2R5/1Q6/8/8/4K3/8 w - - 0 1");

    // std::cout << eval(pos) << std::endl;

    // // generateMoves<false>(moves, pos);

    Search search;

    move16 best_move = search.iterSearch(pos, 15, 1000ULL);

    pos.print();
    pos.printFromBitboard();

    printMoveLong(best_move);

    // std::cout << perft(pos, 1) << std::endl;

}