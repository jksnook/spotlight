#include "eval.hpp"

int eval(Position &pos) {
        int early_eval = 0;
        int late_eval = 0;


        int game_phase = 0;

        for (int i = 0; i < 64; i++) {
            int piece = pos.at(i);

            if (piece != NO_PIECE) {
                game_phase += phase_values[piece % 6];
                if (piece < BLACK_PAWN) {
                    early_eval += piece_values[0][piece % 6] + piece_square_tables[piece % 6][0][i ^ 56];
                    late_eval += piece_values[1][piece % 6] + piece_square_tables[piece % 6][0][i ^ 56];
                } else {
                    early_eval -= piece_values[0][piece % 6] + piece_square_tables[piece % 6][0][i];
                    late_eval -= piece_values[1][piece % 6] + piece_square_tables[piece % 6][0][i];
                }
            }
        }

        int total_eval = (early_eval * game_phase + late_eval * (TOTAL_PHASE - game_phase)) / TOTAL_PHASE;

        if (pos.side_to_move == BLACK) {
            total_eval *= -1;
        }

        return total_eval;
}