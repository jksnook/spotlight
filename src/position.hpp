#pragma once

#include <string>
#include <vector>

#include "types.hpp"

namespace Spotlight {

class MoveGenData {
   public:
    MoveGenData();

    bool generated_checkers;
    BitBoard checkers;
    bool generated_enemy_attacks;
    BitBoard enemy_attacks;
    bool generated_pinned_pieces;
    BitBoard pinned_pieces;
};

struct Undo {
    move16 move;
    Piece piece_moved;
    int castle_rights;
    Square en_passant;
    int fifty_move;
    U64 z_key;
    Piece captured_piece;
    bool in_check;
    MoveGenData movegen_data;
};

class Position {
   public:
    Position();
    BitBoard bitboards[15];
    int castle_rights;
    Color side_to_move;
    Square en_passant;
    int fifty_move;
    int half_moves;
    int game_half_moves;
    U64 z_key;
    bool in_check;

    MoveGenData movegen_data;

    std::vector<Undo> history;

    void readFen(std::string fen);
    std::string toFen();
    void print();
    void printFromBitboard();
    U64 generateZobrist();
    bool isTripleRepetition();

    template <bool update_zobrist>
    void movePiece(Square start, Square end, Piece piece);
    template <bool update_zobrist>
    void removePiece(Square square, Piece piece);
    template <bool update_zobrist>
    void placePiece(Square square, Piece piece);

    void makeMove(move16 move);
    void unmakeMove();

    void makeNullMove();
    void unmakeNullMove();

    move16 parseMove(std::string move_string);
    bool zugzwangUnlikely();

    inline Piece at(Square sq) { return board[sq]; };

   private:
    Piece board[64];
};

}  // namespace Spotlight
