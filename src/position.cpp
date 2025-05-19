#include "position.hpp"

#include <cassert>
#include <iostream>
#include <sstream>

#include "move.hpp"
#include "utils.hpp"
#include "zobrist.hpp"

namespace Spotlight {

template <bool update_zobrist>
void Position::movePiece(Square start, Square end, Piece piece) {
    if constexpr (update_zobrist) {
        z_key ^= piece_keys[piece][start];
        z_key ^= piece_keys[piece][end];
    }
    bitboards[piece] ^= setBit(start);
    bitboards[piece] ^= setBit(end);
    bitboards[OCCUPANCY] ^= setBit(start);
    bitboards[OCCUPANCY] ^= setBit(end);

    bitboards[WHITE_OCCUPANCY + side_to_move] ^= setBit(start);
    bitboards[WHITE_OCCUPANCY + side_to_move] ^= setBit(end);

    board[start] = Piece::NO_PIECE;
    board[end] = piece;
}

template <bool update_zobrist>
void Position::removePiece(Square square, Piece piece) {
    if constexpr (update_zobrist) z_key ^= piece_keys[piece][square];
    bitboards[piece] ^= setBit(square);
    bitboards[WHITE_OCCUPANCY] &= ~setBit(square);
    bitboards[BLACK_OCCUPANCY] &= ~setBit(square);
    bitboards[OCCUPANCY] &= ~setBit(square);

    board[square] = Piece::NO_PIECE;
}

template <bool update_zobrist>
void Position::placePiece(Square square, Piece piece) {
    if constexpr (update_zobrist) z_key ^= piece_keys[piece][square];
    bitboards[piece] ^= setBit(square);
    bitboards[WHITE_OCCUPANCY + piece / 6] ^= setBit(square);
    bitboards[OCCUPANCY] ^= setBit(square);

    board[square] = piece;
}

MoveGenData::MoveGenData()
    : generated_checkers(false),
      checkers(0ULL),
      generated_enemy_attacks(false),
      enemy_attacks(0ULL),
      generated_pinned_pieces(false),
      pinned_pieces(0ULL) {}

void Position::readFen(std::string fen) {
    // resetting bitboards to zero
    for (auto &bitboard : bitboards) {
        bitboard = 0ULL;
    };
    for (auto &i : board) {
        i = NO_PIECE;
    }

    movegen_data = MoveGenData();
    history.clear();

    z_key = 0ULL;
    half_moves = 0;
    side_to_move = WHITE;
    en_passant = A1;
    fifty_move = 0;
    in_check = false;

    // read piece positions
    int space_pos = fen.find(' ');
    std::string pieces = fen.substr(0, space_pos);
    int square_index = 56;
    for (const auto &c : pieces) {
        if (isdigit(c)) {
            square_index += (c - '0');
        } else if (c != '/') {
            Piece p = letterToPiece(c);
            bitboards[p] |= (1ULL << square_index);
            board[square_index] = p;
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

    // read castle rights
    fen = fen.substr(space_pos + 1);
    space_pos = fen.find(' ');
    castle_rights = 0;
    for (const auto &c : fen.substr(0, space_pos)) {
        castle_rights |= charToCastleRights(c);
    };
    fen = fen.substr(space_pos + 1);
    space_pos = fen.find(' ');

    // calculate en passant square
    if (fen[0] == '-') {
        en_passant = A1;
    } else {
        en_passant = static_cast<Square>((fen[1] - '1') * 8 + (fen[0] - 'a'));
    };
    fen = fen.substr(space_pos + 1);
    space_pos = fen.find(' ');
    if (isdigit(fen[0])) {
        fifty_move = std::stoi(fen.substr(0, space_pos)) * 2;
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
            Piece piece = board[sq];
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
            Square sq = static_cast<Square>(rank * 8 + file);
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

    for (int i = 0; i < 64; i++) {
        Piece piece = at(static_cast<Square>(i));
        zobrist ^= piece_keys[piece][i];
    }

    zobrist ^= en_passant_keys[en_passant];
    zobrist ^= side_key * side_to_move;
    zobrist ^= castle_rights_keys[castle_rights];

    return zobrist;
}

Position::Position() : in_check(false) {
    // clearHistory();
    for (auto &i : board) {
        i = NO_PIECE;
    }
    readFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

void Position::makeMove(move16 move) {
    assert(move != NULL_MOVE);
    Square start_square = getFromSquare(move);
    Square end_square = getToSquare(move);
    move16 move_type = getMoveType(move);
    Piece piece = at(start_square);

    assert(piece != NO_PIECE);

    Undo undo;
    undo.movegen_data = movegen_data;
    movegen_data = MoveGenData();
    undo.move = move;
    undo.piece_moved = piece;
    undo.en_passant = en_passant;
    undo.fifty_move = fifty_move;
    undo.castle_rights = castle_rights;
    undo.z_key = z_key;
    undo.captured_piece = NO_PIECE;
    undo.in_check = in_check;
    Piece captured_piece = NO_PIECE;
    if (en_passant) z_key ^= en_passant_keys[en_passant];
    en_passant = A1;

    if (getPieceType(piece) == PAWN) {
        fifty_move = 0;
    }

    switch (move_type) {
        case QUIET_MOVE:
            movePiece<true>(start_square, end_square, piece);
            fifty_move++;
            break;
        case CAPTURE_MOVE:
            captured_piece = at(end_square);
            assert(getPieceType(captured_piece) != KING);
            undo.captured_piece = captured_piece;
            removePiece<true>(end_square, captured_piece);
            movePiece<true>(start_square, end_square, piece);
            fifty_move = 0;
            break;
        case DOUBLE_PAWN_PUSH:
            movePiece<true>(start_square, end_square, piece);
            en_passant = prevPawnSquare(end_square, side_to_move);
            if (!(pawn_attacks[side_to_move][en_passant] &
                  bitboards[getPieceID(PAWN, getOtherSide(side_to_move))])) {
                en_passant = A1;
            } else {
                z_key ^= en_passant_keys[en_passant];
            }
            break;
        case QUEEN_CASTLE:
            movePiece<true>(start_square, end_square, piece);
            movePiece<true>(getRelativeSquare(A1, side_to_move),
                            getRelativeSquare(D1, side_to_move), getPieceID(ROOK, side_to_move));
            fifty_move++;
            break;
        case KING_CASTLE:
            assert(getPieceType(piece) == KING);
            movePiece<true>(start_square, end_square, piece);
            movePiece<true>(getRelativeSquare(H1, side_to_move),
                            getRelativeSquare(F1, side_to_move), getPieceID(ROOK, side_to_move));
            fifty_move++;
            break;
        case EN_PASSANT_CAPTURE:
            captured_piece = getPieceID(PAWN, getOtherSide(side_to_move));
            undo.captured_piece = captured_piece;
            removePiece<true>(prevPawnSquare(end_square, side_to_move), captured_piece);
            movePiece<true>(start_square, end_square, piece);
            break;
        case QUEEN_PROMOTION:
            removePiece<true>(start_square, piece);
            placePiece<true>(end_square, getPieceID(QUEEN, side_to_move));
            break;
        case KNIGHT_PROMOTION:
            removePiece<true>(start_square, piece);
            placePiece<true>(end_square, getPieceID(KNIGHT, side_to_move));
            break;
        case BISHOP_PROMOTION:
            removePiece<true>(start_square, piece);
            placePiece<true>(end_square, getPieceID(BISHOP, side_to_move));
            break;
        case ROOK_PROMOTION:
            removePiece<true>(start_square, piece);
            placePiece<true>(end_square, getPieceID(ROOK, side_to_move));
            break;
        case QUEEN_PROMOTION_CAPTURE:
            captured_piece = at(end_square);
            undo.captured_piece = captured_piece;
            removePiece<true>(start_square, piece);
            removePiece<true>(end_square, at(end_square));
            placePiece<true>(end_square, getPieceID(QUEEN, side_to_move));
            break;
        case KNIGHT_PROMOTION_CAPTURE:
            captured_piece = at(end_square);
            undo.captured_piece = captured_piece;
            removePiece<true>(start_square, piece);
            removePiece<true>(end_square, at(end_square));
            placePiece<true>(end_square, getPieceID(KNIGHT, side_to_move));
            break;
        case BISHOP_PROMOTION_CAPTURE:
            captured_piece = at(end_square);
            undo.captured_piece = captured_piece;
            removePiece<true>(start_square, piece);
            removePiece<true>(end_square, at(end_square));
            placePiece<true>(end_square, getPieceID(BISHOP, side_to_move));
            break;
        case ROOK_PROMOTION_CAPTURE:
            captured_piece = at(end_square);
            undo.captured_piece = captured_piece;
            removePiece<true>(start_square, piece);
            removePiece<true>(end_square, at(end_square));
            placePiece<true>(end_square, getPieceID(ROOK, side_to_move));
            break;
        default:
            break;
    }

    if (castle_rights) {
        z_key ^= castle_rights_keys[castle_rights];
        if (castle_rights & WQC) {
            if (!(setBit(A1) & bitboards[WHITE_ROOK]) || !(setBit(E1) & bitboards[WHITE_KING])) {
                castle_rights &= ~WQC;
            }
        }
        if (castle_rights & WKC) {
            if (!(setBit(H1) & bitboards[WHITE_ROOK]) || !(setBit(E1) & bitboards[WHITE_KING])) {
                castle_rights &= ~WKC;
            }
        }
        if (castle_rights & BQC) {
            if ((setBit(A8) & ~bitboards[BLACK_ROOK]) || (setBit(E8) & ~bitboards[BLACK_KING])) {
                castle_rights &= ~BQC;
            }
        }
        if (castle_rights & BKC) {
            if ((setBit(H8) & ~bitboards[BLACK_ROOK]) || (setBit(E8) & ~bitboards[BLACK_KING])) {
                castle_rights &= ~BKC;
            }
        }
        z_key ^= castle_rights_keys[castle_rights];
    }

    z_key ^= side_key;

    history.push_back(undo);
    in_check = false;
    side_to_move = getOtherSide(side_to_move);
    // z_key = generateZobrist();
    half_moves++;

    // assert(z_key == generateZobrist());
}

void Position::unmakeMove() {
    side_to_move = getOtherSide(side_to_move);
    Undo undo = history.back();
    move16 move = undo.move;

    movegen_data = undo.movegen_data;

    Square start_square = getFromSquare(move);
    Square end_square = getToSquare(move);
    move16 move_type = getMoveType(move);
    Piece piece = at(end_square);

    en_passant = undo.en_passant;
    fifty_move = undo.fifty_move;
    castle_rights = undo.castle_rights;
    in_check = undo.in_check;
    half_moves--;

    switch (move_type) {
        case QUIET_MOVE:
            movePiece<false>(end_square, start_square, piece);
            break;
        case CAPTURE_MOVE:
            movePiece<false>(end_square, start_square, piece);
            placePiece<false>(end_square, undo.captured_piece);
            break;
        case DOUBLE_PAWN_PUSH:
            movePiece<false>(end_square, start_square, piece);
            break;
        case QUEEN_CASTLE:
            movePiece<false>(end_square, start_square, piece);
            movePiece<false>(getRelativeSquare(D1, side_to_move),
                             getRelativeSquare(A1, side_to_move), getPieceID(ROOK, side_to_move));
            break;
        case KING_CASTLE:
            movePiece<false>(end_square, start_square, piece);
            movePiece<false>(getRelativeSquare(F1, side_to_move),
                             getRelativeSquare(H1, side_to_move), getPieceID(ROOK, side_to_move));
            break;
        case EN_PASSANT_CAPTURE:
            placePiece<false>(prevPawnSquare(end_square, side_to_move), undo.captured_piece);
            movePiece<false>(end_square, start_square, piece);
            break;
        case QUEEN_PROMOTION:
            removePiece<false>(end_square, getPieceID(QUEEN, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            break;
        case KNIGHT_PROMOTION:
            removePiece<false>(end_square, getPieceID(KNIGHT, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            break;
        case BISHOP_PROMOTION:
            removePiece<false>(end_square, getPieceID(BISHOP, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            break;
        case ROOK_PROMOTION:
            removePiece<false>(end_square, getPieceID(ROOK, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            break;
        case QUEEN_PROMOTION_CAPTURE:
            removePiece<false>(end_square, getPieceID(QUEEN, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            placePiece<false>(end_square, undo.captured_piece);
            break;
        case KNIGHT_PROMOTION_CAPTURE:
            removePiece<false>(end_square, getPieceID(KNIGHT, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            placePiece<false>(end_square, undo.captured_piece);
            break;
        case BISHOP_PROMOTION_CAPTURE:
            removePiece<false>(end_square, getPieceID(BISHOP, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            placePiece<false>(end_square, undo.captured_piece);
            break;
        case ROOK_PROMOTION_CAPTURE:
            removePiece<false>(end_square, getPieceID(ROOK, side_to_move));
            placePiece<false>(start_square, getPieceID(PAWN, side_to_move));
            placePiece<false>(end_square, undo.captured_piece);
            break;
        default:
            break;
    }

    z_key = undo.z_key;
    history.pop_back();
}

move16 Position::parseMove(std::string move_string) {
    move16 move = 0;
    Square start = static_cast<Square>((move_string[0] - 'a') + (move_string[1] - '1') * 8);
    Square end = static_cast<Square>((move_string[2] - 'a') + (move_string[3] - '1') * 8);

    move16 piece = 0UL;
    move16 move_type = 0UL;

    piece = at(start);

    if (at(end) != NO_PIECE) {
        move_type |= CAPTURE_MOVE;
    }

    if ((piece == WHITE_PAWN && ((1ULL << end) & RANK_8)) ||
        (piece == BLACK_PAWN && ((1ULL << end) & RANK_1))) {
        // determine promotion type here
        if (move_string.length() > 4) {
            switch (move_string[4]) {
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
    } else if ((piece == WHITE_PAWN || piece == BLACK_PAWN) && (end == en_passant)) {
        move_type = EN_PASSANT_CAPTURE;
    } else if (piece == WHITE_PAWN && end == start + 16) {
        move_type = DOUBLE_PAWN_PUSH;
    } else if (piece == BLACK_PAWN && end == start - 16) {
        move_type = DOUBLE_PAWN_PUSH;
    } else if (piece == WHITE_KING && start == E1 && end == G1) {
        move_type = KING_CASTLE;
    } else if (piece == WHITE_KING && start == E1 && end == C1) {
        move_type = QUEEN_CASTLE;
    } else if (piece == BLACK_KING && start == E8 && end == G8) {
        move_type = KING_CASTLE;
    } else if (piece == BLACK_KING && start == E8 && end == C8) {
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

    if (en_passant) z_key ^= en_passant_keys[en_passant];

    en_passant = A1;

    side_to_move = getOtherSide(side_to_move);
    z_key ^= side_key;
    half_moves++;
    fifty_move++;
    history.push_back(undo);
}

void Position::unmakeNullMove() {
    side_to_move = getOtherSide(side_to_move);
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
    for (int i = s - 1; i >= s - fifty_move && i >= 0; i--) {
        if (z_key == history[i].z_key) {
            repetitions++;
        }
    }
    return repetitions >= 2;
}

bool Position::zugzwangUnlikely() {
    for (int p = getPieceID(KNIGHT, side_to_move); p < getPieceID(KING, side_to_move); p++) {
        if (bitboards[p]) return true;
    }
    return false;
}

}  // namespace Spotlight
