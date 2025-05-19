#include "tuner.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "eval.hpp"
#include "position.hpp"

/*
Tuning implementation from Andrew Grant's tuning paper:
https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf
*/

namespace Spotlight {

Tuner::Tuner() : weights{}, gradient{} {
    std::ifstream fen_file;
    std::string line;
    Position pos;

    fen_file.open(TUNING_FILE);
    int positions_loaded = 0;

    while (positions_loaded < MAX_POSITIONS && std::getline(fen_file, line)) {
        PositionData entry;
        std::string outcome;
        std::string fen;

        std::size_t start_of_outcome = line.find_last_of(' ') + 1;
        std::size_t end_of_outcome = line.find_last_not_of(" \t\n\r");

        fen = line.substr(0, start_of_outcome - 1);
        outcome = line.substr(start_of_outcome, end_of_outcome - start_of_outcome + 1);

        if (outcome == "0" || outcome == "[0.0]") {
            entry.result = 0.0;
        } else if (outcome == "0.5" || outcome == "[0.5]") {
            entry.result = 0.5;
        } else if (outcome == "1" || outcome == "[1.0]") {
            entry.result = 1.0;
        }

        pos.readFen(fen);

        /*
        To make things easier I just copied my eval code to here. If I make my eval more complex
        I will have to change this.
        */

        int early_eval = 0;
        int late_eval = 0;

        int game_phase = 0;
        std::array<std::array<int, 2>, NUM_PARAMS / 2> coeffs{};

        for (int i = 0; i < 64; i++) {
            Piece piece = pos.at(static_cast<Square>(i));

            if (piece != NO_PIECE) {
                game_phase += phase_values[piece % 6];
                int coeff_idx;
                if (piece < BLACK_PAWN) {
                    coeff_idx = getPieceType(piece) * 64 + (i ^ 56);
                    coeffs[coeff_idx][WHITE]++;
                    early_eval += piece_values[0][getPieceType(piece)] +
                                  piece_square_tables[getPieceType(piece)][0][i ^ 56];
                    late_eval += piece_values[1][getPieceType(piece)] +
                                 piece_square_tables[getPieceType(piece)][1][i ^ 56];
                } else {
                    coeff_idx = getPieceType(piece) * 64 + i;
                    coeffs[coeff_idx][BLACK]++;
                    early_eval -= piece_values[0][getPieceType(piece)] +
                                  piece_square_tables[getPieceType(piece)][0][i];
                    late_eval -= piece_values[1][getPieceType(piece)] +
                                 piece_square_tables[getPieceType(piece)][1][i];
                }
            }
        }

        int total_eval =
            (early_eval * game_phase + late_eval * (TOTAL_PHASE - game_phase)) / TOTAL_PHASE;

        entry.s_eval = eval(pos);
        if (pos.side_to_move == BLACK) {
            entry.s_eval *= -1;
        }
        entry.d_eval = static_cast<double>(total_eval);
        entry.phase = game_phase;

        for (int i = 0; i < NUM_PARAMS / 2; i++) {
            if (coeffs[i][WHITE] - coeffs[i][BLACK] != 0) {
                CoeffData data;
                data.wcoef = coeffs[i][WHITE];
                data.bcoef = coeffs[i][BLACK];
                data.index = i;
                entry.active_coeffs.push_back(data);
            }
        }

        t_positions.push_back(entry);
        positions_loaded++;
    }
    std::cout << positions_loaded << " Positions Loaded\n";
}

void Tuner::forward() {
    for (auto &p : t_positions) {
        double early_eval = 0.0;
        double late_eval = 0.0;

        for (auto &c : p.active_coeffs) {
            early_eval += (c.wcoef - c.bcoef) * weights[c.index][0];
            late_eval += (c.wcoef - c.bcoef) * weights[c.index][1];
        }

        p.d_eval =
            (early_eval * p.phase + late_eval * (TOTAL_PHASE - p.phase)) / TOTAL_PHASE + p.s_eval;
    }
}

double Tuner::sigmoid(double k, double eval) { return 1.0 / (1.0 + exp(-k * eval / 400.0)); }

double Tuner::staticEvaluationError(double k) {
    double total_error = 0.0;

    for (auto &p : t_positions) {
        total_error += pow((p.result - sigmoid(k, p.s_eval)), 2);
    }

    return total_error / static_cast<double>(t_positions.size());
}

double Tuner::evaluationError(double k) {
    double total_error = 0.0;

    for (auto &p : t_positions) {
        total_error += pow((p.result - sigmoid(k, p.d_eval)), 2);
    }

    return total_error / static_cast<double>(t_positions.size());
}

double Tuner::computeOptimalK() {
    double start = 0.0, end = 10.0, step = 1.0;
    double curr = start;
    double error;
    double best = staticEvaluationError(start);
    std::cout << "Computing optimal K\n";

    for (int i = 0; i < K_PRECISION - 1; i++) {
        curr = start - step;
        while (curr < end) {
            curr = curr + step;
            error = staticEvaluationError(curr);
            if (error <= best) {
                start = curr;
                best = error;
            }
        }

        std::cout << start << "\n";
        if (i < K_PRECISION) {
            end = start + step;
            start = start - step;
            step = step / 10.0;
        }
    }

    return start;
}

void Tuner::zeroGrad() {
    for (auto &a : gradient) {
        a[0] = 0.0;
        a[1] = 0.0;
    }
}

void Tuner::calculateGradient() {
    for (auto &p : t_positions) {
        double s = sigmoid(k_param, p.d_eval);
        double derr = (p.result - s);
        double ds = s * (1 - s);
        double mg_phase = static_cast<double>(p.phase) / TOTAL_PHASE;
        double eg_phase = 1.0 - mg_phase;
        for (auto &t : p.active_coeffs) {
            double temp = (t.wcoef - t.bcoef) * derr * ds;
            gradient[t.index][0] += temp * mg_phase;
            gradient[t.index][1] += temp * eg_phase;
        }
    }
}

void Tuner::updateWeights(double lr) {
    for (int i = 0; i < PSQ_ARRAY_SIZE; i++) {
        weights[i][0] -= gradient[i][0] * lr;
        weights[i][1] -= gradient[i][1] * lr;
    }
}

void Tuner::run() {
    k_param = computeOptimalK();
    std::array<std::array<double, 2>, PSQ_ARRAY_SIZE> ada_grad{};

    std::cout << "Tuning\n";
    for (int epoch = 0; epoch < MAX_EPOCHS; epoch++) {
        zeroGrad();
        forward();
        calculateGradient();
        for (int i = 0; i < PSQ_ARRAY_SIZE; i++) {
            ada_grad[i][0] += pow(2.0 * gradient[i][0] / NUM_PARAMS, 2.0);
            ada_grad[i][1] += pow(2.0 * gradient[i][1] / NUM_PARAMS, 2.0);

            weights[i][0] += (k_param * 2.0 / NUM_PARAMS) * gradient[i][0] *
                             (LEARNING_RATE / sqrt(1e-8 + ada_grad[i][0]));
            weights[i][1] += (k_param * 2.0 / NUM_PARAMS) * gradient[i][1] *
                             (LEARNING_RATE / sqrt(1e-8 + ada_grad[i][1]));
        }

        if (epoch % REPORT_INTERVAL == 0) {
            std::cout << "Epoch " << epoch << "/" << MAX_EPOCHS
                      << " Evaluation Error: " << evaluationError(k_param) << "\n";
        }
    }
}

void Tuner::printWeights() {
    for (int piece = PAWN; piece <= KING; piece++) {
        std::cout << "{\n";
        for (int phase = 0; phase <= 1; phase++) {
            std::cout << "    {\n";
            for (int rank = 0; rank < 8; rank++) {
                std::cout << "       ";
                for (int file = 0; file < 8; file++) {
                    int sq = rank * 8 + file;
                    int idx = piece * 64 + sq;
                    int w = std::round(weights[idx][phase]) + piece_square_tables[piece][phase][sq];
                    int spaces = 3;
                    if (w != 0) {
                        spaces = 3 - floor(log10(abs(w)));
                        if (w < 0) {
                            spaces -= 1;
                        }
                    }
                    for (int i = 0; i < spaces; i++) {
                        std::cout << " ";
                    }
                    std::cout << w << ",";
                }
                std::cout << "\n";
            }
            std::cout << "    },\n";
        }
        std::cout << "},\n";
    }
}

void Tuner::outputToFile() {
    std::ofstream out_file;
    out_file.open(TUNING_PARAMS_FILE);
    if (!out_file.is_open()) return;
    for (int piece = PAWN; piece <= KING; piece++) {
        out_file << "{\n";
        for (int phase = 0; phase <= 1; phase++) {
            out_file << "    {\n";
            for (int rank = 0; rank < 8; rank++) {
                out_file << "       ";
                for (int file = 0; file < 8; file++) {
                    int sq = rank * 8 + file;
                    int idx = piece * 64 + sq;
                    int w = std::round(weights[idx][phase]) + piece_square_tables[piece][phase][sq];
                    int spaces = 3;
                    if (w != 0) {
                        spaces = 3 - floor(log10(abs(w)));
                        if (w < 0) {
                            spaces -= 1;
                        }
                    }
                    for (int i = 0; i < spaces; i++) {
                        out_file << " ";
                    }
                    out_file << w << ",";
                }
                out_file << "\n";
            }
            out_file << "    },\n";
        }
        out_file << "},\n";
    }
    out_file.close();
}

}  // namespace Spotlight
