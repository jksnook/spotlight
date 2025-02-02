#include "position.hpp"
#include "utils.hpp"
#include "zobrist.hpp"
#include "bitboards.hpp"
#include "movegen.hpp"

#include <iostream>

void Position::readFen(std::string fen) {
    //resetting bitboards to zero
    for (auto &bitboard: bitboards) {
        bitboard = 0ULL;
    };
    for (auto &i: board) {
        i = NO_PIECE;
    }

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

void Position::printFromBitboard() {
    std::cout << "+---+---+---+---+---+---+---+---+\n";
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            bool square_empty = true;
            for (int piece = white_pawn; piece <= black_king; piece++) {
                if (bitboards[piece] & (1ULL << (rank * 8 + file))) {
                    square_empty = false;
                    std::cout << "| " << piece_to_letter_map[piece] << ' ';
                }
            }
            if (square_empty) {
                std::cout << "|   ";
            }
        }
        std::cout << "|\n+---+---+---+---+---+---+---+---+\n";
    }
    std::cout << "castle rights " << castle_rights << std::endl;
};

U64 Position::generateZobrist() {
    U64 zobrist = 0ULL;
    U64 bitboard;

    // for (int piece = white_pawn; piece <= black_king; piece++){

    //     bitboard = bitboards[piece];

    //     while (bitboard) {
    //     zobrist ^= piece_keys[piece][popLSB(bitboard)];
    //     }
    // }

    for (int i = 0; i < 64; i++) {
        int piece = at(i);
        zobrist ^= piece_keys[piece][i];
    }

    zobrist ^= en_passant_keys[en_passant];
    zobrist ^= side_key * side_to_move;
    zobrist ^= castle_rights_keys[castle_rights];

    return zobrist;
}

Position::Position(): in_check(false) {
    for (auto &i: board) {
        i = NO_PIECE;
    }
    readFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

int Position::at(int sq) {
    return board[sq];
}

void Position::movePiece(int start, int end, int piece_type) {
    bitboards[piece_type] ^= setBit(start);
    bitboards[piece_type] ^= setBit(end);
    bitboards[occupancy] ^= setBit(start);
    bitboards[occupancy] ^= setBit(end);

    bitboards[white_occupancy + side_to_move] ^= setBit(start);
    bitboards[white_occupancy + side_to_move] ^= setBit(end);

    board[start] = NO_PIECE;
    board[end] = piece_type;
}

void Position::removePiece(int square, int piece_type) {
    bitboards[piece_type] ^= setBit(square);
    bitboards[white_occupancy] &= ~setBit(square);
    bitboards[black_occupancy] &= ~setBit(square);
    bitboards[occupancy] &= ~setBit(square);

    board[square] = NO_PIECE;
}

void Position::placePiece(int square, int piece_type) {
    bitboards[piece_type] ^= setBit(square);
    bitboards[white_occupancy + piece_type / 6] ^= setBit(square);
    bitboards[occupancy] ^= setBit(square);

    board[square] = piece_type;
}

void Position::makeMove(move16 move) {
    int start_square = getFromSquare(move);
    int end_square = getToSquare(move);
    int move_type = getMoveType(move);
    int piece_type = at(start_square);

    Undo undo;
    undo.move = move;
    undo.en_passant = en_passant;
    undo.fifty_move = fifty_move;
    undo.castle_rights = castle_rights;
    undo.z_key = z_key;
    undo.captured_piece = NO_PIECE;
    int captured_piece = NO_PIECE;
    en_passant = 0;

    switch (move_type)
    {
    case quiet_move:
        movePiece(start_square, end_square, piece_type);
        fifty_move++;
        break;
    case capture_move:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(end_square, captured_piece);
        movePiece(start_square, end_square, piece_type);
        fifty_move = 0;
        break;
    case double_pawn_push:
        movePiece(start_square, end_square, piece_type);
        en_passant = end_square + (side_to_move * 8) + (side_to_move - 1) * 8;
        fifty_move = 0;
        break;
    case queen_castle:
        movePiece(start_square, end_square, piece_type);
        movePiece(a1 + a8 * side_to_move, d1 + a8 * side_to_move, white_rook + 6 * side_to_move);
        fifty_move++;
        break;
    case king_castle:
        movePiece(start_square, end_square, piece_type);
        movePiece(h1 + a8 * side_to_move, f1 + a8 * side_to_move, white_rook + 6 * side_to_move);
        fifty_move++;
        break;
    case en_passant_capture:
        captured_piece = white_pawn + 6 * (side_to_move ^ 1);
        undo.captured_piece = captured_piece;
        removePiece(end_square + 8 * side_to_move - 8 * (1 - side_to_move), captured_piece);
        movePiece(start_square, end_square, piece_type);
        fifty_move = 0;
        break;
    case queen_promotion:
        removePiece(start_square, piece_type);
        placePiece(end_square, white_queen + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case knight_promotion:
        removePiece(start_square, piece_type);
        placePiece(end_square, white_knight + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case bishop_promotion:
        removePiece(start_square, piece_type);
        placePiece(end_square, white_bishop + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case rook_promotion:
        removePiece(start_square, piece_type);
        placePiece(end_square, white_rook + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case queen_promotion_capture:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece_type);
        removePiece(end_square, at(end_square));
        placePiece(end_square, white_queen + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case knight_promotion_capture:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece_type);
        removePiece(end_square, at(end_square));
        placePiece(end_square, white_knight + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case bishop_promotion_capture:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece_type);
        removePiece(end_square, at(end_square));
        placePiece(end_square, white_bishop + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case rook_promotion_capture:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece_type);
        removePiece(end_square, at(end_square));
        placePiece(end_square, white_rook + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    default:
        break;
    }

    if (castle_rights & wqc) {
        if(!(setBit(a1) & bitboards[white_rook]) || !(1ULL << e1 & bitboards[white_king])) {
            castle_rights &= ~wqc;
        }
    }
    if (castle_rights & wkc) {
        if(!(1ULL << h1 & bitboards[white_rook]) || !(1ULL << e1 & bitboards[white_king])) {
            castle_rights &= ~wkc;
        }
    }
    if (castle_rights & bqc) {
        if((1ULL << a8 & ~bitboards[black_rook]) || (1ULL << e8 & ~bitboards[black_king])) {
            castle_rights &= ~bqc;
        }
    }
    if (castle_rights & bkc) {
        if((1ULL << h8 & ~bitboards[black_rook]) || (1ULL << e8 & ~bitboards[black_king])) {
            castle_rights &= ~bkc;
        }
    }

    history.push_back(undo);
    side_to_move ^= 1;
    z_key = generateZobrist();
}

void Position::unmakeMove() {
    side_to_move ^= 1;
    Undo undo = history.back();
    move16 move = undo.move;

    int start_square = getFromSquare(move);
    int end_square = getToSquare(move);
    int move_type = getMoveType(move);
    int piece_type = at(end_square);

    en_passant = undo.en_passant;
    fifty_move = undo.fifty_move;
    castle_rights = undo.castle_rights;
    z_key = undo.z_key;

    switch (move_type)
    {
    case quiet_move:
        movePiece(end_square, start_square, piece_type);
        break;
    case capture_move:
        movePiece(end_square, start_square, piece_type);
        placePiece(end_square, undo.captured_piece);
        break;
    case double_pawn_push:
        movePiece(end_square, start_square, piece_type);
        break;
    case queen_castle:
        movePiece(end_square, start_square, piece_type);
        movePiece(d1 + a8 * side_to_move, a1 + a8 * side_to_move, white_rook + 6 * side_to_move);
        break;
    case king_castle:
        movePiece(end_square, start_square, piece_type);
        movePiece(f1 + a8 * side_to_move, h1 + a8 * side_to_move, white_rook + 6 * side_to_move);
        break;
    case en_passant_capture:
        placePiece(end_square + 8 * side_to_move - 8 * (1 - side_to_move), undo.captured_piece);
        movePiece(end_square, start_square, piece_type);
        fifty_move = 0;
        break;
    case queen_promotion:
        removePiece(end_square, white_queen + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        break;
    case knight_promotion:
        removePiece(end_square, white_knight + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        break;
    case bishop_promotion:
        removePiece(end_square, white_bishop + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case rook_promotion:
        removePiece(end_square, white_rook + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        fifty_move = 0;
        break;
    case queen_promotion_capture:
        removePiece(end_square, white_queen + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        placePiece(end_square, undo.captured_piece);
        break;
    case knight_promotion_capture:
        removePiece(end_square, white_knight + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        placePiece(end_square, undo.captured_piece);
        break;
    case bishop_promotion_capture:
        removePiece(end_square, white_bishop + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        placePiece(end_square, undo.captured_piece);
        break;
    case rook_promotion_capture:
        removePiece(end_square, white_rook + black_pawn * side_to_move);
        placePiece(start_square, white_pawn + black_pawn * side_to_move);
        placePiece(end_square, undo.captured_piece);
        break;
    default:
        break;
    }

    history.pop_back();
}

move16 Position::parseMove(std::string move_string) {
    move16 move = 0;
    int start = (move_string[0] - 'a') + (move_string[1] - '1') * 8;
    int end = (move_string[2] - 'a') + (move_string[3] - '1') * 8;

    move16 piece_type = 0UL;
    move16 move_type = 0UL;

    piece_type = at(start);

    if (at(end) != NO_PIECE) {
        move_type |= capture_move;
    }

    if ((piece_type == white_pawn && ((1ULL << end) & rank_8)) || (piece_type == black_pawn && ((1ULL << end) & rank_1))) {
    // determine promotion type here
        if (move_string.length() > 4) {
            switch (move_string[4])
            {
            case 'q':
                move_type |= queen_promotion;
                break;
            case 'n':
                move_type |= knight_promotion;
                break;
            case 'b':
                move_type |= bishop_promotion;
                break;
            case 'r':
                move_type |= rook_promotion;
                break;
            case 'Q':
                move_type |= queen_promotion;
                break;
            case 'N':
                move_type |= knight_promotion;
                break;
            case 'B':
                move_type |= bishop_promotion;
                break;
            case 'R':
                move_type |= rook_promotion;
                break;
            default:
                break;
            }
        }
    }  else if ((piece_type == white_pawn || piece_type == black_pawn) && (end == en_passant)) {
        move_type = en_passant_capture;
    } else if (piece_type == white_pawn && end == start + 16) {
        move_type = double_pawn_push;
    } else if (piece_type == black_pawn && end == start - 16) {
        move_type = double_pawn_push;
    } else if (piece_type == white_king && start == e1 && end == g1) {
        move_type = king_castle;
    } else if (piece_type == white_king && start == e1 && end == c1) {
        move_type = queen_castle;
    } else if (piece_type == black_king && start == e8 && end == g8) {
        move_type = king_castle;
    } else if (piece_type == black_king && start == e8 && end == c8) {
        move_type = queen_castle;
    }

    move |= end << 6;
    move |= start;
    move |= move_type << 12;

    return move;
}

bool Position::isTripleRepetition() {
    const int s = static_cast<int>(history.size());
    if (s == 0) return false;
    int repetitions = 0;
    for (int i = s - 1; i >= s - fifty_move; i--) {
        if (z_key == history[i].z_key) {
            repetitions++;
        }
    }
    return repetitions >= 2;
}

