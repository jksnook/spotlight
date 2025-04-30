#include "test.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "move.hpp"
// #include "moveorder.hpp"
#include "movegen.hpp"
// #include "search.hpp"
// #include "moparams.hpp"
// #include "movepicker.hpp"

#include <cassert>
#include <iostream>
#include <chrono>

namespace Spotlight {

void testPerft() {
    Position pos;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    U64 total_nodes = 0;
    U64 nodes;

    nodes = perft(pos, 5);
    total_nodes += nodes;
    assert(nodes == 4865609ULL);

    pos.readFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    nodes = perft(pos, 5);
    total_nodes += nodes;
    assert(nodes == 193690690ULL);

    pos.readFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ");
    nodes = perft(pos, 5);
    total_nodes += nodes;
    assert(nodes == 674624ULL);

    pos.readFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    nodes = perft(pos, 5);
    total_nodes += nodes;
    assert(nodes == 89941194ULL);

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);

    int nps = total_nodes * 1000 / elapsed_time.count();
    // int nps = 0;

    std::cout << total_nodes << " nodes visited at " << nps << " nps \n";
}

} //namespace Spotlight
