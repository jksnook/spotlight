#include "eval.hpp"

#include "bitboards.hpp"

namespace Spotlight {

int eval(Position &pos) {
    int early_eval = 0;
    int late_eval = 0;
    int game_phase = 0;
    BitBoard occ = pos.bitboards[WHITE_OCCUPANCY];

    while (occ) {
        Square i = popLSB(occ);
        PieceType pt = getPieceType(pos.at(i));
        game_phase += phase_values[pt];
        early_eval += piece_values[0][pt] + piece_square_tables[pt][0][i ^ 56];
        late_eval += piece_values[1][pt] + piece_square_tables[pt][1][i ^ 56];
    }

    occ = pos.bitboards[BLACK_OCCUPANCY];

    while (occ) {
        Square i = popLSB(occ);
        PieceType pt = getPieceType(pos.at(i));
        game_phase += phase_values[pt];
        early_eval -= piece_values[0][pt] + piece_square_tables[pt][0][i];
        late_eval -= piece_values[1][pt] + piece_square_tables[pt][1][i];
    }

    int total_eval =
        (early_eval * game_phase + late_eval * (TOTAL_PHASE - game_phase)) / TOTAL_PHASE;

    // assert(game_phase == TOTAL_PHASE || total_eval != early_eval);

    if (pos.side_to_move == BLACK) {
        total_eval *= -1;
    }

    return total_eval;
}

}  // namespace Spotlight
