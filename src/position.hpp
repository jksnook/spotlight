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

        template<bool white_to_move>
        void movePiece(int start, int end, int piece_type);

        template<bool white_to_move>
        void makeMove(move16 &move);

        int at(int sq);

    private:
        int board[NUM_SQUARES];
};

template<bool white_to_move>
void Position::movePiece(int start, int end, int piece_type) {
    bitboards[piece_type] ^= setBit(start);
    bitboards[piece_type] ^= setBit(end);
    bitboards[occupancy] ^= setBit(start);
    bitboards[occupancy] ^= setBit(end);

    if constexpr(white_to_move) {
        bitboards[white_occupancy] ^= setBit(start);
        bitboards[white_occupancy] ^= setBit(end);
    } else {
        bitboards[black_occupancy] ^= setBit(start);
        bitboards[black_occupancy] ^= setBit(end);
    }

    board[start] = NO_PIECE;
    board[end] = piece_type;
}

template<bool white_to_move>
void Position::makeMove(move16 &move) {
    int start_square = get_from_square(move);
    int end_square = get_to_square(move);
    int move_type = get_move_type(move);
    int piece_type = at(start_square);

    Undo undo;
    undo.move = move;
    undo.en_passant = en_passant;
    undo.fifty_move = fifty_move;
    undo.castle_rights = castle_rights;
    undo.z_key = z_key;
    en_passant = 0;

    switch (move_type)
    {
    case quiet_move:
        movePiece<white_to_move>(start_square, end_square, piece_type);
        if (piece_type == white_pawn || piece_type == black_pawn) {
            fifty_move = 0;
        } else {
            fifty_move++;
        }
        break;
    
    default:
        break;
    }

    side_to_move ^= 1;
    z_key = generateZobrist();
    history.push_back(undo);
}