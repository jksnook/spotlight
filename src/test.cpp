#include "test.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "move.hpp"
#include "moveorder.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "tunables.hpp"

#include <cassert>
#include <iostream>
#include <chrono>

void runTests() {
    testSee();
    testPerft();
    testCheck();

    std::cout << "Tests Passed" << std::endl;
}

void testSee() {
    Position pos;

    pos.readFen("1r2k3/8/6r1/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move16 move = encodeMove(C5, B6, EN_PASSANT_CAPTURE);
    int score = see(pos, move);
    int correct_score = SEE_MARGIN * SEE_MULTIPLIER;
    assert(score == correct_score);

    pos.readFen("4k3/p7/6r1/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, EN_PASSANT_CAPTURE);
    score = see(pos, move);
    correct_score = SEE_MARGIN * SEE_MULTIPLIER;
    assert(score == correct_score);

    pos.readFen("4k3/p7/8/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, EN_PASSANT_CAPTURE);
    score = see(pos, move);
    correct_score = (SEE_VALUES[PAWN] + SEE_MARGIN) * SEE_MULTIPLIER;
    assert(score == correct_score);

    pos.readFen("4k3/8/8/1pP5/8/8/8/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, EN_PASSANT_CAPTURE);
    score = see(pos, move);
    correct_score = (SEE_VALUES[PAWN] + SEE_MARGIN) * SEE_MULTIPLIER;
    assert(score == correct_score);

    pos.readFen("4k3/8/8/8/8/8/2p5/1R2K3 b - - 0 1");
    move = encodeMove(C2, B1, QUEEN_PROMOTION_CAPTURE);
    score = see(pos, move);
    correct_score = (SEE_VALUES[QUEEN] + SEE_VALUES[ROOK] - SEE_VALUES[PAWN] + SEE_MARGIN) * SEE_MULTIPLIER;
    assert(score == correct_score);
}

void testCheck() {
    Position pos;
    pos.readFen("5k2/p7/8/2B3b1/8/8/7P/5K2 w - - 0 1");
    assert(inCheck(pos) == false);

    pos.readFen("5k2/p7/8/2B3b1/8/8/7P/5K2 b - - 0 1");
    assert(inCheck(pos) == true);

    pos.readFen("5k2/p7/B7/8/8/3b4/7P/5K2 w - - 0 1");
    assert(inCheck(pos) == true);
}

void testPerft() {
    Position pos;
    assert(perft(pos, 5) == 4865609ULL);

    pos.readFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    assert(perft(pos, 5) == 193690690ULL);

    pos.readFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - ");
    assert(perft(pos, 5) == 674624ULL);

    pos.readFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    assert(perft(pos, 5) == 89941194ULL);
}

void testSearch() {
    Position pos;
    Search search;

    U64 nodes;
    U64 q_nodes;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    for (const auto &fen: TEST_POSITIONS) {
        pos.clearHistory();
        search.clearTT();
        pos.readFen(fen);
        SearchResult r = search.iterSearch(pos, 7, 10000);
        nodes += search.total_nodes;
        q_nodes += search.q_nodes;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);

    int nps = nodes * 1000 / elapsed_time.count();

    std::cout << nodes << " nodes searched at " << nps << " nps " << q_nodes << " quiescence nodes\n";
}