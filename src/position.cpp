#include "position.hpp"
#include "utils.hpp"
#include "zobrist.hpp"
#include "bitboards.hpp"

#include <iostream>

void Position::readFen(std::string fen) {
    //resetting bitboards to zero
    for (auto &bitboard: bitboards) {
        bitboard = 0ULL;
    };

    z_key = 0ULL;
    ply = 0;
    side_to_move = 0;
    en_passant = 0;
    fifty_move = 0;


    //read piece positions
    int space_pos = fen.find(' ');
    std::string pieces = fen.substr(0, space_pos);
    int square_index = 56;
    for (const auto &c: pieces) {
        if (isdigit(c)) {
            square_index += (c - '0');
        } else if (c != '/') {
            bitboards[letter_piece_map[c]] |= (1ULL << square_index);
            board[square_index] = letter_piece_map[c];
            square_index++;
        } else {
            square_index -= 16;
        };
    };
    for (int i = white_pawn; i <= black_king; i++) {
        bitboards[occupancy] |= bitboards[i];
    };
    for (int i = white_pawn; i <= white_king; i++) {
        bitboards[white_occupancy] |= bitboards[i];
    };
    for (int i = black_pawn; i <= black_king; i++) {
        bitboards[black_occupancy] |= bitboards[i];
    };

    // read side to move
    fen = fen.substr(space_pos + 1);
    space_pos = fen.find(' ');
    if (fen[0] == 'w') {
        side_to_move = WHITE;
    } else {
        side_to_move = BLACK;
    };

    //read castle rights
    fen = fen.substr(space_pos + 1);
    space_pos = fen.find(' ');
    castle_rights = 0;
    for (const auto &c: fen.substr(0, space_pos)) {
        castle_rights |= castle_rights_map[c];
    };
    fen = fen.substr(space_pos + 1);
    space_pos = fen.find(' ');


    // calculate en passant square
    if (fen[0] == '-') {
        en_passant = 0;
    } else {
        en_passant = (fen[1] - '1') * 8 + (fen[0] - 'a');
    };
    fen = fen.substr(space_pos + 1);
    space_pos = fen.find(' ');
    if (isdigit(fen[0])) {
        fifty_move = std::stoi(fen.substr(0, space_pos));
    } else {
        fifty_move = 0;
    }
    fen = fen.substr(space_pos + 1);
    if (isdigit(fen[0])) {
        ply = 2 * (std::stoi(fen) - 1);
    } else {
        ply = 0;
    }

    z_key = generateZobrist();

};

void Position::print() {
    std::cout << "+---+---+---+---+---+---+---+---+\n";
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            // bool square_empty = true;
            // for (int piece = white_pawn; piece <= black_king; piece++) {
            //     if (bitboards[piece] & (1ULL << (rank * 8 + file))) {
            //         square_empty = false;
            //         std::cout << "| " << piece_to_letter_map[piece] << ' ';
            //     }
            // }
            // if (square_empty) {
            //     std::cout << "|   ";
            // }
            int sq = (rank * 8 + file);
            int piece = at(sq);
            if (piece != NO_PIECE) {
                std::cout << "| " << piece_to_letter_map[piece] << ' ';
            } else {
                std::cout << "|   ";
            }
        }
        std::cout << "|\n+---+---+---+---+---+---+---+---+\n";
    }
};

U64 Position::generateZobrist() {
    U64 zobrist = 0ULL;
    U64 bitboard;

    for (int piece = white_pawn; piece <= black_king; piece++){

        bitboard = bitboards[piece];

        while (bitboard) {
        zobrist ^= piece_keys[piece][popLSB(bitboard)];
        }
    }

    zobrist ^= en_passant_keys[en_passant];
    zobrist ^= side_key * side_to_move;
    zobrist ^= castle_rights_keys[castle_rights];

    return zobrist;
}

Position::Position() {
    for (auto &i: board) {
        i = 15;
    }
    readFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

int Position::at(int sq) {
    return board[sq];
}
