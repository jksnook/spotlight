#include "test.hpp"
#include "utils.hpp"
#include "position.hpp"
#include "move.hpp"
#include "moveorder.hpp"

#include <cassert>
#include <iostream>

void runTests() {
    testSee();

    std::cout << "Tests Passed" << std::endl;
}

void testSee() {
    Position pos;

    pos.readFen("1r2k3/8/6r1/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move16 move = encodeMove(C5, B6, en_passant_capture);
    int score = see(pos, move);
    int correct_score = 0;
    assert(score == correct_score);

    pos.readFen("4k3/p7/6r1/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, en_passant_capture);
    score = see(pos, move);
    correct_score = 0;
    assert(score == correct_score);

    pos.readFen("4k3/p7/8/1pP5/8/8/1R6/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, en_passant_capture);
    score = see(pos, move);
    correct_score = see_values[PAWN] * 1000;
    assert(score == correct_score);

    pos.readFen("4k3/8/8/1pP5/8/8/8/4K3 w - b6 0 2");
    move = encodeMove(C5, B6, en_passant_capture);
    score = see(pos, move);
    correct_score = see_values[PAWN] * 1000;
    assert(score == correct_score);

    pos.readFen("4k3/8/8/8/8/8/2p5/1R2K3 b - - 0 1");
    move = encodeMove(C2, B1, queen_promotion_capture);
    score = see(pos, move);
    correct_score = (see_values[QUEEN] + see_values[ROOK] - see_values[PAWN]) * 1000;
    assert(score == correct_score);
}