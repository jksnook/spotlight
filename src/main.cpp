#include <iostream>
#include <string>

#include "bitboards.hpp"
#include "zobrist.hpp"
#include "position.hpp"
#include "movegen.hpp"

int main() {
    initMoves();
    initMagics();
    initZobrist();

    Position pos;

    pos.print();

    MoveList moves;

    generateMoves<true, false>(moves, pos);

    std::cout << moves.size() << std::endl;

    for (const auto &m: moves) {
        printMove(m);
    }

    printMove(encodeMove(0,0,0));

}