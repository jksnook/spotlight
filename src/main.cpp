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

int main(int argc, char* argv[]) {
    initMoves();
    initMagics();
    initZobrist();

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
        selfplay(150);
    }
}