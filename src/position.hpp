#pragma once

#include "types.hpp"
#include "utils.hpp"
#include "move.hpp"
#include <vector>
#include <string>
#include <sstream>

const int MAX_HISTORY = 1000;

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
        int half_moves;
        int game_half_moves;
        U64 z_key;
        bool in_check;

        std::vector<Undo> history;
        int history_table[2][64][64];

        void updateHistoryTable(int from, int to, int depth);
        void clearHistoryTable();

        void readFen(std::string fen);
        std::string toFen();
        void print();
        void printFromBitboard();
        U64 generateZobrist();
        bool isTripleRepetition();

        void movePiece(int start, int end, int piece_type);
        void removePiece(int square, int piece_type);
        void placePiece(int square, int piece_type);

        void makeMove(move16 move);
        void unmakeMove();

        void makeNullMove();
        void unmakeNullMove();

        move16 parseMove(std::string move_string);

        int at(int sq);

    private:
        int board[NUM_SQUARES];
};
