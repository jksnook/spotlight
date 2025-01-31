#include <iostream>
#include <string>

#include "bitboards.hpp"
#include "zobrist.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "uci.hpp"

int main() {
    initMoves();
    initMagics();
    initZobrist();

    UCI uci;

    uci.loop();

    // Position pos;

    // MoveList moves;

    // pos.readFen("r3k2r/p1ppqpb1/bn2pnp1/1B1PN3/1p2P3/2N2Q1p/PPPB1PPP/R3K2R b KQkq - 1 1");

    // // generateMoves<false>(moves, pos);

    // std::cout << perft(pos, 1) << std::endl;

}