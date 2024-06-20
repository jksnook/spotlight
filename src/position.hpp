#pragma once

#include "types.hpp"
#include <vector>

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
        int ply;

        std::vector<Undo> history;


    private:

};