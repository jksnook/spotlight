#include <iostream>
#include <string>

#include "bitboards.hpp"
#include "zobrist.hpp"
#include "position.hpp"

int main() {
    initMoves();
    initMagics();
    initZobrist();

    Position pos;

    pos.print();

}