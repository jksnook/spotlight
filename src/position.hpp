#pragma once

#include "types.hpp"
#include "utils.hpp"
#include <vector>
#include <string>

struct Undo {
    public:
        move16 move;
        int castle_rights;
        int en_passant;
        int fifty_move;
        U64 z_key;
};

class Position {
    public:
        Position();
        U64 bitboards[15];
        int castle_rights;
        int side_to_move;
        int en_passant;
        int fifty_move;
        int ply;
        U64 z_key;


        std::vector<Undo> history;

        void readFen(std::string fen);
        void print();
        U64 generateZobrist();

    private:

};