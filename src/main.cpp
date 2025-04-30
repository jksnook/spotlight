#include "types.hpp"
#include "position.hpp"
#include "zobrist.hpp"
#include "move.hpp"

using namespace Spotlight;

int main(int argc, char* argv[]) {

    Position pos;

    pos.print();

    pos.makeMove(encodeMove(E2, E4, DOUBLE_PAWN_PUSH));

    pos.print();

}