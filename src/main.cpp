#include <iostream>
#include <string>

#include "bitboards.hpp"

int main() {
    initMoves();
    initMagics();

    std::cout << "Hello World!" << std::endl;

    for (const auto magic: rook_magic_numbers) {
        std::cout << "0x" << std::hex << magic << "ULL,\n";
    }

    std::cout << "-----------------------------------\n";

    for (const auto magic: bishop_magic_numbers) {
        std::cout << "0x" << std::hex << magic << "ULL,\n";
    }
}