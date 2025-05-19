#include "datagen.hpp"
#include "eval.hpp"
#include "move.hpp"
#include "position.hpp"
#include "test.hpp"
#include "tuner.hpp"
#include "types.hpp"
#include "uci.hpp"
#include "zobrist.hpp"

using namespace Spotlight;

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
        if (argc == 5) {
            selfplay(std::stoi(argv[2]), std::stoi(argv[3]), std::stoi(argv[4]));
        } else {
            selfplay(100, NUM_THREADS, BASE_NODE_COUNT);
        }
    }
}