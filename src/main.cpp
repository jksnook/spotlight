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
#include "threads.hpp"

int main(int argc, char* argv[]) {
    initMoves();
    initMagics();
    initZobrist();

    // Threads t(4);

    // Position pos;

    // t.timeSearch(pos, 1000ULL);

    // t.finishSearch();

    // pos.makeMove(encodeMove(E2, E4, DOUBLE_PAWN_PUSH));

    // t.timeSearch(pos, 1000ULL);

    // std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    // t.exitThreads();

    if (argc == 1) {
        UCI uci;
        uci.loop();
    } else if (static_cast<std::string>(argv[1]) == "searchtest") {
        testSearch();
    } else if (static_cast<std::string>(argv[1]) == "fulltest") {
        runTests();
    } else if (static_cast<std::string>(argv[1]) == "tune") {
        Tuner tuner;

        tuner.run();
        tuner.printWeights();
        tuner.outputToFile();
    } else if (static_cast<std::string>(argv[1]) == "datagen") {
        selfplay(1000);
    }
    // position fen 2kr1b1r/ppp2ppp/2n2q2/1B1p1b2/3Pn3/2P2N2/PP1N1PPP/R1BQR1K1 b - - 7 11
}
