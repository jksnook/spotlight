#include <iostream>
#include <string>

#include "bitboards.hpp"
#include "zobrist.hpp"
#include "position.hpp"
#include "movegen.hpp"
#include "uci.hpp"

int main() {
    initMoves();
    initMagics();
    initZobrist();

    UCI uci;

    uci.loop();
}