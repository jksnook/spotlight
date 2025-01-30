#pragma once

#include "types.hpp"
#include "utils.hpp"
#include "move.hpp"
#include <vector>
#include <string>

struct Undo {
    public:
        move16 move;
        int castle_rights;
        int en_passant;
        int fifty_move;
        U64 z_key;
        int captured_piece;
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
        void printFromBitboard();
        U64 generateZobrist();

        void movePiece(int start, int end, int piece_type);
        void removePiece(int square, int piece_type);
        void placePiece(int square, int piece_type);

        void makeMove(move16 move);
        void unmakeMove();

        move16 parseMove(std::string move_string);

        int at(int sq);

    private:
        int board[NUM_SQUARES];
};
