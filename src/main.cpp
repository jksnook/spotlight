#include "types.hpp"
#include "position.hpp"
#include "zobrist.hpp"
#include "move.hpp"
#include "test.hpp"
#include "eval.hpp"

using namespace Spotlight;

int main(int argc, char* argv[]) {
    initMoves();
    initMagics();
    initZobrist();

    // Position pos;

    // pos.readFen("rnbqkbnr/pppppppp/8/8/P7/8/1PPPPPPP/RNBQKBNR b KQkq - 0 1");

    // pos.makeMove(encodeMove(B7, B5, DOUBLE_PAWN_PUSH));
    // pos.print();
    // pos.printFromBitboard();
    // pos.unmakeMove();
    // pos.print();
    // pos.printFromBitboard();


    // perft()

    // testPerft();

    testSee();
    testCheck();
    testMovePicker();

    testSearch();

}