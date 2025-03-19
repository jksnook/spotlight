#include "position.hpp"
#include "utils.hpp"
#include "zobrist.hpp"
#include "bitboards.hpp"
#include "movegen.hpp"
#include "tunables.hpp"

#include <iostream>
#include <sstream>
#include <algorithm>


void Position::readFen(std::string fen) {
    //resetting bitboards to zero
    for (auto &bitboard: bitboards) {
        bitboard = 0ULL;
    };
    for (auto &i: board) {
        i = NO_PIECE;
    }

    z_key = 0ULL;
    half_moves = 0;
    side_to_move = 0;
    en_passant = 0;
    fifty_move = 0;
    in_check = false;


    //read piece positions
    int space_pos = fen.find(' ');
    std::string pieces = fen.substr(0, space_pos);
    int square_index = 56;
    for (const auto &c: pieces) {
        if (isdigit(c)) {
            square_index += (c - '0');
        } else if (c != '/') {
            bitboards[LETTER_PIECE_MAP[c]] |= (1ULL << square_index);
            board[square_index] = LETTER_PIECE_MAP[c];
            square_index++;
        } else {
            square_index -= 16;
        };
    };
    for (int i = WHITE_PAWN; i <= BLACK_KING; i++) {
        bitboards[OCCUPANCY] |= bitboards[i];
    };
    for (int i = WHITE_PAWN; i <= WHITE_KING; i++) {
        bitboards[WHITE_OCCUPANCY] |= bitboards[i];
    };
    for (int i = BLACK_PAWN; i <= BLACK_KING; i++) {
        bitboards[BLACK_OCCUPANCY] |= bitboards[i];
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
        castle_rights |= CASTLE_RIGHTS_MAP[c];
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
        half_moves = 2 * (std::stoi(fen) - 1);
    } else {
        half_moves = 0;
    }
    game_half_moves = half_moves;

    z_key = generateZobrist();

}

std::string Position::toFen() {
    std::stringstream fen;
    for (int rank = 7; rank >= 0; rank--) {
        int empties = 0;
        for (int file = 0; file < 8; file++) {
            int sq = rank * 8 + file;
            int piece = board[sq];
            if (piece == NO_PIECE) {
                empties++;
            } else {
                if (empties > 0) {
                    fen << empties;
                }
                empties = 0;
                fen << PIECE_TO_LETTER_MAP[piece];
            }
        }
        if (empties > 0) {
            fen << empties;
        }
        if (rank > 0) {
            fen << '/';
        }
    }

    if (side_to_move == WHITE) {
        fen << " w";
    } else {
        fen << " b";
    }

    fen << " ";

    if (castle_rights) {
        if (castle_rights & WKC) {
            fen << "K";
        }
        if (castle_rights & WQC) {
            fen << "Q";
        }
        if (castle_rights & BKC) {
            fen << "k";
        }
        if (castle_rights & BQC) {
            fen << "q";
        }
    } else {
        fen << "-";
    }

    if (en_passant) {
        fen << " " << SQUARE_NAMES[en_passant];
    } else {
        fen << " -";
    }

    fen << " " << fifty_move;
    fen << " " << (half_moves / 2 + 1);

    return fen.str();
}

void Position::print() {
    std::cout << "+---+---+---+---+---+---+---+---+\n";
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 0; file < 8; file++) {
            int sq = (rank * 8 + file);
            int piece = at(sq);
            if (piece != NO_PIECE) {
                std::cout << "| " << PIECE_TO_LETTER_MAP[piece] << ' ';
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
            for (int piece = WHITE_PAWN; piece <= BLACK_KING; piece++) {
                if (bitboards[piece] & (1ULL << (rank * 8 + file))) {
                    square_empty = false;
                    std::cout << "| " << PIECE_TO_LETTER_MAP[piece] << ' ';
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
    clearHistory();
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
    bitboards[OCCUPANCY] ^= setBit(start);
    bitboards[OCCUPANCY] ^= setBit(end);

    bitboards[WHITE_OCCUPANCY + side_to_move] ^= setBit(start);
    bitboards[WHITE_OCCUPANCY + side_to_move] ^= setBit(end);

    board[start] = NO_PIECE;
    board[end] = piece_type;
}

void Position::removePiece(int square, int piece_type) {
    bitboards[piece_type] ^= setBit(square);
    bitboards[WHITE_OCCUPANCY] &= ~setBit(square);
    bitboards[BLACK_OCCUPANCY] &= ~setBit(square);
    bitboards[OCCUPANCY] &= ~setBit(square);

    board[square] = NO_PIECE;
}

void Position::placePiece(int square, int piece_type) {
    bitboards[piece_type] ^= setBit(square);
    bitboards[WHITE_OCCUPANCY + piece_type / 6] ^= setBit(square);
    bitboards[OCCUPANCY] ^= setBit(square);

    board[square] = piece_type;
}

void Position::makeMove(move16 move) {
    assert(move != 0);
    int start_square = getFromSquare(move);
    int end_square = getToSquare(move);
    int move_type = getMoveType(move);
    int piece = at(start_square);

    assert(piece != NO_PIECE);

    Undo undo;
    undo.move = move;
    undo.piece_moved = piece;
    undo.en_passant = en_passant;
    undo.fifty_move = fifty_move;
    undo.castle_rights = castle_rights;
    undo.z_key = z_key;
    undo.captured_piece = NO_PIECE;
    undo.in_check = in_check;
    int captured_piece = NO_PIECE;
    en_passant = 0;

    if (piece % 6 == PAWN) {
        fifty_move = 0;
    }

    switch (move_type)
    {
    case QUIET_MOVE:
        movePiece(start_square, end_square, piece);
        fifty_move++;
        break;
    case CAPTURE_MOVE:
        captured_piece = at(end_square);
        assert(captured_piece % 6 != KING);
        undo.captured_piece = captured_piece;
        removePiece(end_square, captured_piece);
        movePiece(start_square, end_square, piece);
        fifty_move = 0;
        break;
    case DOUBLE_PAWN_PUSH:
        movePiece(start_square, end_square, piece);
        en_passant = end_square + (side_to_move * 8) + (side_to_move - 1) * 8;
        if (!(pawn_attacks[side_to_move][en_passant] & bitboards[WHITE_PAWN + BLACK_PAWN * (side_to_move ^ 1)])) {
            en_passant = 0;
        }
        break;
    case QUEEN_CASTLE:
        movePiece(start_square, end_square, piece);
        movePiece(a1 + a8 * side_to_move, d1 + a8 * side_to_move, WHITE_ROOK + 6 * side_to_move);
        fifty_move++;
        break;
    case KING_CASTLE:
        movePiece(start_square, end_square, piece);
        movePiece(h1 + a8 * side_to_move, f1 + a8 * side_to_move, WHITE_ROOK + 6 * side_to_move);
        fifty_move++;
        break;
    case EN_PASSANT_CAPTURE:
        captured_piece = WHITE_PAWN + 6 * (side_to_move ^ 1);
        undo.captured_piece = captured_piece;
        removePiece(end_square + 8 * side_to_move - 8 * (1 - side_to_move), captured_piece);
        movePiece(start_square, end_square, piece);
        break;
    case QUEEN_PROMOTION:
        removePiece(start_square, piece);
        placePiece(end_square, WHITE_QUEEN + BLACK_PAWN * side_to_move);
        break;
    case KNIGHT_PROMOTION:
        removePiece(start_square, piece);
        placePiece(end_square, WHITE_KNIGHT + BLACK_PAWN * side_to_move);
        break;
    case BISHOP_PROMOTION:
        removePiece(start_square, piece);
        placePiece(end_square, WHITE_BISHOP + BLACK_PAWN * side_to_move);
        break;
    case ROOK_PROMOTION:
        removePiece(start_square, piece);
        placePiece(end_square, WHITE_ROOK + BLACK_PAWN * side_to_move);
        break;
    case QUEEN_PROMOTION_CAPTURE:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece);
        removePiece(end_square, at(end_square));
        placePiece(end_square, WHITE_QUEEN + BLACK_PAWN * side_to_move);
        break;
    case KNIGHT_PROMOTION_CAPTURE:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece);
        removePiece(end_square, at(end_square));
        placePiece(end_square, WHITE_KNIGHT + BLACK_PAWN * side_to_move);
        break;
    case BISHOP_PROMOTION_CAPTURE:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece);
        removePiece(end_square, at(end_square));
        placePiece(end_square, WHITE_BISHOP + BLACK_PAWN * side_to_move);
        break;
    case ROOK_PROMOTION_CAPTURE:
        captured_piece = at(end_square);
        undo.captured_piece = captured_piece;
        removePiece(start_square, piece);
        removePiece(end_square, at(end_square));
        placePiece(end_square, WHITE_ROOK + BLACK_PAWN * side_to_move);
        break;
    default:
        break;
    }

    if (castle_rights & WQC) {
        if(!(setBit(a1) & bitboards[WHITE_ROOK]) || !(setBit(e1) & bitboards[WHITE_KING])) {
            castle_rights &= ~WQC;
        }
    }
    if (castle_rights & WKC) {
        if(!(1ULL << h1 & bitboards[WHITE_ROOK]) || !(setBit(e1) & bitboards[WHITE_KING])) {
            castle_rights &= ~WKC;
        }
    }
    if (castle_rights & BQC) {
        if((1ULL << a8 & ~bitboards[BLACK_ROOK]) || (1ULL << e8 & ~bitboards[BLACK_KING])) {
            castle_rights &= ~BQC;
        }
    }
    if (castle_rights & BKC) {
        if((1ULL << h8 & ~bitboards[BLACK_ROOK]) || (1ULL << e8 & ~bitboards[BLACK_KING])) {
            castle_rights &= ~BKC;
        }
    }

    history.push_back(undo);
    in_check = false;
    side_to_move ^= 1;
    z_key = generateZobrist();
    half_moves++;
}

void Position::unmakeMove() {
    side_to_move ^= 1;
    Undo undo = history.back();
    move16 move = undo.move;

    int start_square = getFromSquare(move);
    int end_square = getToSquare(move);
    int move_type = getMoveType(move);
    int piece = at(end_square);

    en_passant = undo.en_passant;
    fifty_move = undo.fifty_move;
    castle_rights = undo.castle_rights;
    z_key = undo.z_key;
    in_check = undo.in_check;
    half_moves--;

    switch (move_type)
    {
    case QUIET_MOVE:
        movePiece(end_square, start_square, piece);
        break;
    case CAPTURE_MOVE:
        movePiece(end_square, start_square, piece);
        placePiece(end_square, undo.captured_piece);
        break;
    case DOUBLE_PAWN_PUSH:
        movePiece(end_square, start_square, piece);
        break;
    case QUEEN_CASTLE:
        movePiece(end_square, start_square, piece);
        movePiece(d1 + a8 * side_to_move, a1 + a8 * side_to_move, WHITE_ROOK + 6 * side_to_move);
        break;
    case KING_CASTLE:
        movePiece(end_square, start_square, piece);
        movePiece(f1 + a8 * side_to_move, h1 + a8 * side_to_move, WHITE_ROOK + 6 * side_to_move);
        break;
    case EN_PASSANT_CAPTURE:
        placePiece(end_square + 8 * side_to_move - 8 * (1 - side_to_move), undo.captured_piece);
        movePiece(end_square, start_square, piece);
        break;
    case QUEEN_PROMOTION:
        removePiece(end_square, WHITE_QUEEN + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
        break;
    case KNIGHT_PROMOTION:
        removePiece(end_square, WHITE_KNIGHT + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
        break;
    case BISHOP_PROMOTION:
        removePiece(end_square, WHITE_BISHOP + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
        break;
    case ROOK_PROMOTION:
        removePiece(end_square, WHITE_ROOK + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
        break;
    case QUEEN_PROMOTION_CAPTURE:
        removePiece(end_square, WHITE_QUEEN + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
        placePiece(end_square, undo.captured_piece);
        break;
    case KNIGHT_PROMOTION_CAPTURE:
        removePiece(end_square, WHITE_KNIGHT + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
        placePiece(end_square, undo.captured_piece);
        break;
    case BISHOP_PROMOTION_CAPTURE:
        removePiece(end_square, WHITE_BISHOP + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
        placePiece(end_square, undo.captured_piece);
        break;
    case ROOK_PROMOTION_CAPTURE:
        removePiece(end_square, WHITE_ROOK + BLACK_PAWN * side_to_move);
        placePiece(start_square, WHITE_PAWN + BLACK_PAWN * side_to_move);
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

    move16 piece = 0UL;
    move16 move_type = 0UL;

    piece = at(start);

    if (at(end) != NO_PIECE) {
        move_type |= CAPTURE_MOVE;
    }

    if ((piece == WHITE_PAWN && ((1ULL << end) & RANK_8)) || (piece == BLACK_PAWN && ((1ULL << end) & RANK_1))) {
    // determine promotion type here
        if (move_string.length() > 4) {
            switch (move_string[4])
            {
            case 'q':
                move_type |= QUEEN_PROMOTION;
                break;
            case 'n':
                move_type |= KNIGHT_PROMOTION;
                break;
            case 'b':
                move_type |= BISHOP_PROMOTION;
                break;
            case 'r':
                move_type |= ROOK_PROMOTION;
                break;
            case 'Q':
                move_type |= QUEEN_PROMOTION;
                break;
            case 'N':
                move_type |= KNIGHT_PROMOTION;
                break;
            case 'B':
                move_type |= BISHOP_PROMOTION;
                break;
            case 'R':
                move_type |= ROOK_PROMOTION;
                break;
            default:
                break;
            }
        }
    }  else if ((piece == WHITE_PAWN || piece == BLACK_PAWN) && (end == en_passant)) {
        move_type = EN_PASSANT_CAPTURE;
    } else if (piece == WHITE_PAWN && end == start + 16) {
        move_type = DOUBLE_PAWN_PUSH;
    } else if (piece == BLACK_PAWN && end == start - 16) {
        move_type = DOUBLE_PAWN_PUSH;
    } else if (piece == WHITE_KING && start == e1 && end == g1) {
        move_type = KING_CASTLE;
    } else if (piece == WHITE_KING && start == e1 && end == c1) {
        move_type = QUEEN_CASTLE;
    } else if (piece == BLACK_KING && start == e8 && end == g8) {
        move_type = KING_CASTLE;
    } else if (piece == BLACK_KING && start == e8 && end == c8) {
        move_type = QUEEN_CASTLE;
    }

    move |= end << 6;
    move |= start;
    move |= move_type << 12;

    return move;
}

void Position::makeNullMove() {
    Undo undo;
    undo.move = 0;
    undo.en_passant = en_passant;
    undo.fifty_move = fifty_move;
    undo.z_key = z_key;
    en_passant = 0;

    side_to_move ^= 1;
    z_key = generateZobrist();
    half_moves++;
    fifty_move++;
    history.push_back(undo);
}

void Position::unmakeNullMove() {
    side_to_move ^= 1;
    Undo undo = history.back();

    en_passant = undo.en_passant;
    fifty_move = undo.fifty_move;
    z_key = undo.z_key;
    half_moves--;

    history.pop_back();
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

void Position::clearHistory() {
    for (auto &side: history_table) {
        for (auto &start: side) {
            for (auto &end: start) {
                end = 0;
            }
        }
    }
}

void Position::updateHistory(int from, int to, int bonus) {
    history_table[side_to_move][from][to] += bonus - abs(bonus) * history_table[side_to_move][from][to] / MAX_HISTORY;
}
