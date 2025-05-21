#include "test.hpp"

#include <cassert>
#include <chrono>
#include <iostream>

#include "move.hpp"
#include "movegen.hpp"
#include "movepicker.hpp"
#include "position.hpp"
#include "search.hpp"
#include "see.hpp"
#include "utils.hpp"

namespace Spotlight {

constexpr std::array TEST_POSITIONS = {
    "r1b1kb1r/2pp1ppp/1np1q3/p3P3/2P5/1P6/PB1NQPPP/R3KB1R b KQkq - 0 1",
    "r2q1rk1/1ppnbppp/p2p1nb1/3Pp3/2P1P1P1/2N2N1P/PPB1QP2/R1B2RK1 b - -",
    "r1bq1rk1/1pp2pbp/p1np1np1/3Pp3/2P1P3/2N1BP2/PP4PP/R1NQKB1R b KQ - 1 9",
    "rnbqr1k1/1p3pbp/p2p1np1/2pP4/4P3/2N5/PP1NBPPP/R1BQ1RK1 w - - 1 11",
    "rnbqkb1r/pppp1ppp/5n2/4p3/4PP2/2N5/PPPP2PP/R1BQKBNR b KQkq f3 1 3",
    "r1bqk1nr/pppnbppp/3p4/8/2BNP3/8/PPP2PPP/RNBQK2R w KQkq - 2 6",
    "rnbq1b1r/ppp2kpp/3p1n2/8/3PP3/8/PPP2PPP/RNBQKB1R b KQ d3 1 5",
    "rnbqkb1r/pppp1ppp/3n4/8/2BQ4/5N2/PPP2PPP/RNB2RK1 b kq - 1 6",
    "1r3rk1/5pb1/p2p2p1/Q1n1q2p/1NP1P3/3p1P1B/PP1R3P/1K2R3 b - -",
    "2b2rk1/p1p4p/2p1p1p1/br2N1Q1/1p2q3/8/PB3PPP/3R1RK1 w - -",
    "2k1rb1r/ppp3pp/2np1q2/5b2/2B2P2/2P1BQ2/PP1N1P1P/2KR3R b - -",
    "r4rk1/1bq1bp1p/4p1p1/p2p4/3BnP2/1N1B3R/PPP3PP/R2Q2K1 w - -",
    "8/8/8/1p5r/p1p1k1pN/P2pBpP1/1P1K1P2/8 b - - ",
    "2b5/1r6/2kBp1p1/p2pP1P1/2pP4/1pP3K1/1R3P2/8 b - - ",
    "r4rk1/1b1nqp1p/p5p1/1p2PQ2/2p5/5N2/PP3PPP/R1BR2K1 w - - ",
    "1R2rq1k/2p3p1/Q2p1pPp/8/4P3/8/P1r3PP/1R4K1 w - - ",
    "r5k1/pQp2qpp/8/4pbN1/3P4/6P1/PPr4P/1K1R3R b - -",
    "1k1r4/pp1r1pp1/4n1p1/2R5/2Pp1qP1/3P2QP/P4PB1/1R4K1 w - -",
    "8/6k1/5pp1/Q6p/5P2/6PK/P4q1P/8 b - -",
    "2b4k/p1b2p2/2p2q2/3p1PNp/3P2R1/3B4/P1Q2PKP/4r3 w - -",
    "2rq1rk1/pp3ppp/2n2b2/4NR2/3P4/PB5Q/1P4PP/3R2K1 w - - ",
    "k5r1/p4b2/2P5/5p2/3P1P2/4QBrq/P5P1/4R1K1 w - -",
    "3r3r/p4pk1/5Rp1/3q4/1p1P2RQ/5N2/P1P4P/2b4K w - - ",
    "4r1k1/pq3p1p/2p1r1p1/2Q1p3/3nN1P1/1P6/P1P2P1P/3RR1K1 w - - "};

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
    assert(seeGe(pos, move, 0));
    assert(!seeGe(pos, move, 1));

    pos.readFen("4k3/p7/6r1/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, EN_PASSANT_CAPTURE);
    score = see(pos, move);
    correct_score = SEE_MARGIN * SEE_MULTIPLIER;
    assert(score == correct_score);
    assert(seeGe(pos, move, 0));
    assert(!seeGe(pos, move, 1));

    pos.readFen("4k3/p7/8/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, EN_PASSANT_CAPTURE);
    score = see(pos, move);
    correct_score = (SEE_VALUES[PAWN] + SEE_MARGIN) * SEE_MULTIPLIER;
    assert(score == correct_score);
    assert(seeGe(pos, move, SEE_VALUES[PAWN]));
    assert(!seeGe(pos, move, SEE_VALUES[PAWN] + 1));

    pos.readFen("4k3/8/8/1pP5/8/8/8/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, EN_PASSANT_CAPTURE);
    score = see(pos, move);
    correct_score = (SEE_VALUES[PAWN] + SEE_MARGIN) * SEE_MULTIPLIER;
    assert(score == correct_score);
    assert(seeGe(pos, move, SEE_VALUES[PAWN]));
    assert(!seeGe(pos, move, SEE_VALUES[PAWN] + 1));

    pos.readFen("4k3/8/8/8/8/8/2p5/1R2K3 b - - 0 1");
    move = encodeMove(C2, B1, QUEEN_PROMOTION_CAPTURE);
    score = see(pos, move);
    correct_score =
        (SEE_VALUES[QUEEN] + SEE_VALUES[ROOK] - SEE_VALUES[PAWN] + SEE_MARGIN) * SEE_MULTIPLIER;
    assert(score == correct_score);
    assert(seeGe(pos, move, SEE_VALUES[QUEEN] + SEE_VALUES[ROOK] - SEE_VALUES[PAWN]));
    assert(!seeGe(pos, move, SEE_VALUES[QUEEN] + SEE_VALUES[ROOK] - SEE_VALUES[PAWN] + 1));

    pos.readFen("k2q4/3q4/3q4/8/8/3Q4/3Q4/K2Q4 w - - 0 1");
    move = encodeMove(D3, D6, CAPTURE_MOVE);
    score = see(pos, move);
    correct_score = (SEE_VALUES[QUEEN] + SEE_MARGIN) * SEE_MULTIPLIER;
    assert(score == correct_score);
    assert(seeGe(pos, move, SEE_VALUES[QUEEN]));
    assert(!seeGe(pos, move, SEE_VALUES[QUEEN] + 1));

    pos.readFen("k7/8/8/r7/8/5N2/8/K7 w - - 0 1");
    move = encodeMove(F3, E5, QUIET_MOVE);
    assert(seeGe(pos, move, -SEE_VALUES[KNIGHT]));
    assert(!seeGe(pos, move, -SEE_VALUES[KNIGHT] + 1));

    pos.readFen("k2r3q/8/8/8/8/5N2/K7/3R2Q1 w - - 0 1");
    move = encodeMove(F3, D4, QUIET_MOVE);
    assert(seeGe(pos, move, 0));
    assert(!seeGe(pos, move, 1));
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
    // constexpr std::array PERFT_POSITIONS = {
    // "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    // "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
    // "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1 ",
    // "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    // "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"};

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

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int nps = total_nodes * 1000 / elapsed_time.count();
    // int nps = 0;

    std::cout << total_nodes << " nodes visited at " << nps << " nps \n";
}

void testSearch() {

    Position pos;
    TT tt;
    std::atomic<bool> is_stopped(false);

    Search search(&tt, &is_stopped, [&search]() { return search.nodes_searched; });

    U64 nodes = 0ULL;
    U64 q_nodes = 0ULL;

    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    for (const auto &fen : TEST_POSITIONS) {
        search.clearHistory();
        search.clearTT();
        pos.readFen(fen);
        SearchResult r = search.timeSearch(pos, 15, 100000ULL);
        std::cout << "result move" << r.move << "\n";
        nodes += search.nodes_searched;
        q_nodes += search.q_nodes;
    }

    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

    auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int nps = nodes * 1000 / elapsed_time.count();

    std::cout << nodes << " nodes searched at " << nps << " nps " << q_nodes
              << " quiescence nodes\n";
}

void testMovePicker() {
    Position pos;
    int hist[2][64][64];

    for (auto &side : hist) {
        for (auto &start : side) {
            for (auto &end : start) {
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

    while (picker.getNextMove()) {
        continue;
    }

    std::random_device r;
    std::mt19937 myRandom(r());

    for (const auto &p : TEST_POSITIONS) {
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
        while ((m = picker.getNextMove())) {
            if (!(getMoveType(m) & CAPTURE_MOVE)) {
                assert(m == killer_1);
                break;
            }
        }

        assert(picker.getNextMove() == killer_2);

        while (picker.getNextMove()) {
            continue;
        }
    }
}

U64 testLegalPerftHelper(Position &pos, int depth) {
    if (depth == 0) {
        return 1;
    }

    U64 nodes = 0;

    for (move16 move = 0; move < 0xffff; move++) {
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

    for (move16 move = 0; move < 0xffff; move++) {
        if (isLegal(move, pos)) {
            pos.makeMove(move);
            int nodes_this_move = testLegalPerftHelper(pos, depth - 1);
            pos.unmakeMove();
            nodes += nodes_this_move;
            std::cout << moveToString(move) << ": " << nodes_this_move << std::endl;
            // std::cout << moveToString(move) << " " << move_type_map[getMoveType(move)] << ": " <<
            // nodes_this_move << std::endl;
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

}  // namespace Spotlight
