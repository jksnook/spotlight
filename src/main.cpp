#include <iostream>
#include <string>

#include "bitboards.hpp"
#include "zobrist.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "uci.hpp"
#include "eval.hpp"
#include "search.hpp"
#include "datagen.hpp"
#include "tuner.hpp"
#include "test.hpp"

int main() {
    initMoves();
    initMagics();
    initZobrist();

    // testSearch();

    UCI uci;

    uci.loop();

    // selfplay(30);

    // Tuner tuner;

    // tuner.run();

    // tuner.printWeights();

    // tuner.outputToFile();

    // tuner.k_param = tuner.computeOptimalK();

    // std::cout << tuner.k_param << std::endl;

    // tuner.forward();

    // std::cout << tuner.evaluationError(tuner.k_param) << std::endl;

    // tuner.calculateGradient();

    // tuner.updateWeights(0.1);

    // tuner.forward();

    // std::cout << tuner.evaluationError(tuner.k_param) << std::endl;

    // Position pos;

    // MoveList moves;

    // pos.readFen("rnbq1rk1/p5b1/2p1pp2/7p/3PN1pB/6P1/PPP1BP1P/R2QR1K1 w - - 7 16");

    // Search search;

    // pos.print();

    // std::cout << search.qScore(pos) << std::endl;
    // std::cout << eval(pos) << std::endl;

    // problem with fen r1bqkbnr/p1p5/2n3pp/1pP1pp2/1P1pPP2/B2P4/P2KN1PP/RN1Q1B1R b kq - 1 9

    // incorrect eval at position startpos moves e2e4 b8c6 b1c3 e7e5 g1f3 g8f6 f1d3 f8c5 e1g1 e8g8 c3d5 f8e8 c2c4 g8h8 f3g5 h8g8 g5f3 g8h8 f3g5 h8g8

    // r1b1kb1r/2pp1ppp/1np1q3/p3P3/2P5/1P6/PB1NQPPP/R3KB1R b KQkq - 0 1

}