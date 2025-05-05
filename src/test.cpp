#include "test.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "move.hpp"
#include "see.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "movepicker.hpp"

#include <cassert>
#include <iostream>
#include <chrono>

namespace Spotlight {

void runTests() {
    // testMoveVerification();
    testMovePicker();
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

void testSearch() {
    Position pos;
    TT tt;
    std::atomic<bool> is_stopped(false);

    Search search(&tt, &is_stopped);

    U64 nodes;
    U64 q_nodes;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    for (const auto &fen: TEST_POSITIONS) {
        search.clearHistory();
        search.clearTT();
        pos.readFen(fen);
        SearchResult r = search.timeSearch(pos, 15, 100000);
        nodes += search.total_nodes;
        q_nodes += search.q_nodes;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds> (end - start);

    int nps = nodes * 1000 / elapsed_time.count();

    std::cout << nodes << " nodes searched at " << nps << " nps " << q_nodes << " quiescence nodes\n";
}

void testMovePicker() {
    Position pos;
    int hist[2][64][64];

    for (auto &side: hist) {
        for (auto &start: side) {
            for (auto &end: start) {
                end = 0;
            }
        }
    }

    move16 tt_move = encodeMove(C2, C4, DOUBLE_PAWN_PUSH);
    move16 killer_1 = encodeMove(D2, D4, DOUBLE_PAWN_PUSH);
    move16 killer_2 = encodeMove(E2, E4, DOUBLE_PAWN_PUSH);

    MovePicker picker(pos, &hist, tt_move, killer_1, killer_2);

    assert(picker.getNextMove() == tt_move);
    move16 move = picker.getNextMove();
    assert(move == killer_1);
    assert(picker.getNextMove() == killer_2);

    while (picker.getNextMove())
    {
        continue;
    }

    std::random_device r;
    std::mt19937 myRandom(r());

    for (const auto &p: TEST_POSITIONS) {
        pos.readFen(p);
        MoveList moves;

        if (pos.side_to_move == WHITE) {
            generateMovesSided<WHITE, LEGAL>(moves, pos);
        } else {
            generateMovesSided<BLACK, LEGAL>(moves, pos);
        }

        MoveList quiets;
        MoveList captures;
        generateQuietMoves(quiets, pos);
        generateNoisyMoves(captures, pos);
        
        killer_1 = 0;
        killer_2 = 0;

        move16 tt_move = moves[myRandom() % moves.size()].move;
        do {
            killer_1 = quiets[myRandom() % quiets.size()].move;
        } while (killer_1 == tt_move);
        do {
            killer_2 = quiets[myRandom() % quiets.size()].move;
        } while (killer_2 == tt_move || killer_2 == killer_1);

        MovePicker picker(pos, &hist, tt_move, killer_1, killer_2);

        assert(picker.getNextMove() == tt_move);

        move16 m;
        while (m = picker.getNextMove()) {
            if (!(getMoveType(m) & CAPTURE_MOVE)) {
                assert(m == killer_1);
                break;
            }
        }

        assert(picker.getNextMove() == killer_2);

        while (picker.getNextMove())
        {
            continue;
        }

    }
}

U64 testLegalPerftHelper(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;

    for(move16 move = 0; move < 0xffff; move++) {
        if (isLegal(move, pos)) {
            pos.makeMove(move);
            nodes += testLegalPerftHelper(pos, depth - 1);
            pos.unmakeMove();
        }
    }

    return nodes;
};


// Iterate through (almost) all possible move encodings and play the legal ones
U64 testLegalPerft(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;

    for(move16 move = 0; move < 0xffff; move++) {
        if (isLegal(move, pos)) {
            pos.makeMove(move);
            int nodes_this_move = testLegalPerftHelper(pos, depth - 1);
            pos.unmakeMove();
            nodes += nodes_this_move;
            std::cout << moveToString(move) << ": " << nodes_this_move << std::endl;
            // std::cout << moveToString(move) << " " << move_type_map[getMoveType(move)] << ": " << nodes_this_move << std::endl;
        }
    }

    return nodes;
};


void testMoveVerification() {
    Position pos;

    pos.readFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    int n = testLegalPerft(pos, 1);
    std::cout << n << " nodes\n";
    assert(n == 48);
}

} //namespace Spotlight
