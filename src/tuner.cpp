#include "tuner.hpp"
#include "position.hpp"
#include "eval.hpp"

#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

Tuner::Tuner(): weights{}, gradient{} {
    std::ifstream fen_file;
    std::string line;
    Position pos;

    fen_file.open("selfplay.txt");

    while(std::getline(fen_file, line)) {
        PositionData entry;
        std::string outcome;
        std::string fen;

        std::size_t i = line.find_last_of(' ');

        fen = line.substr(0, i);
        outcome = line.substr(i + 1);

        if (outcome == "0") {
            entry.result = 0.0;
        } else if (outcome == "0.5") {
            entry.result = 0.5;
        } else if (outcome == "1") {
            entry.result = 1.0;
        }

        pos.readFen(fen);

        int early_eval = 0;
        int late_eval = 0;


        int game_phase = 0;
        std::array<std::array<int, 2>, NUM_WEIGHTS / 2> coeffs{};

        for (int i = 0; i < 64; i++) {
            int piece = pos.at(i);

            if (piece != NO_PIECE) {
                game_phase += phase_values[piece % 6];
                int coeff_idx;
                if (piece < BLACK_PAWN) {
                    coeff_idx = (piece % 6) * 64 + (i ^ 56);
                    coeffs[coeff_idx][WHITE]++;
                    early_eval += piece_values[0][piece % 6] + piece_square_tables[piece % 6][0][i ^ 56];
                    late_eval += piece_values[1][piece % 6] + piece_square_tables[piece % 6][0][i ^ 56];
                } else {
                    coeff_idx = (piece % 6) * 64 + i;
                    coeffs[coeff_idx][BLACK]++;
                    early_eval -= piece_values[0][piece % 6] + piece_square_tables[piece % 6][0][i];
                    late_eval -= piece_values[1][piece % 6] + piece_square_tables[piece % 6][0][i];
                }
            }
        }

        int total_eval = (early_eval * game_phase + late_eval * (TOTAL_PHASE - game_phase)) / TOTAL_PHASE;

        // if (pos.side_to_move == BLACK) {
        //     total_eval *= -1;
        // }

        entry.s_eval = total_eval;
        entry.eval = static_cast<double>(total_eval);
        entry.phase = game_phase;

        for (int i = 0; i < NUM_WEIGHTS / 2; i++) {
            if (coeffs[i][WHITE] - coeffs[i][BLACK] != 0) {
                WeightData data;
                data.wcoef = coeffs[i][WHITE];
                data.bcoef = coeffs[i][BLACK];
                data.index = i;
                entry.active_coeffs.push_back(data);
            }
        }

        // for (int piece = 0; piece < BLACK_PAWN; piece++) {
        //     for (int phase = 0; phase < 2; phase++) {
        //         for (int square = 0; square < 64; square++) {
        //             int weight_idx = piece * 64 + square;
        //             weights[weight_idx][phase] = piece_square_tables[piece][phase][square];
        //         }
        //     }
        // }

        t_positions.push_back(entry);

        // std::cout << pos.toFen() << " " << outcome << " " << entry.eval << "\n";
    }
}

void Tuner::forward() {
    for (auto &p: t_positions) {
        double early_eval = 0.0;
        double late_eval = 0.0;

        for (auto &c: p.active_coeffs) {
            // need to include phase somehow
            early_eval += (c.wcoef - c.bcoef) * weights[c.index][0];
            late_eval += (c.wcoef - c.bcoef) * weights[c.index][1];
        }

        p.eval = (early_eval * p.phase + late_eval * (TOTAL_PHASE - p.phase)) / TOTAL_PHASE + p.s_eval;
        // std::cout << p.eval << "\n";
    }
}

double Tuner::sigmoid(double k, double eval) {
    return 1.0 / (1.0 + exp(-k * eval / 400.0));
}

double Tuner::evaluationError(double k) {
    double total_error = 0.0;

    for (auto &p: t_positions) {
        total_error += pow((p.result - sigmoid(k, p.eval)), 2);
    }

    return total_error / static_cast<double>(t_positions.size());
}

double Tuner::computeOptimalK() {
    double start = 0.0, end = 10.0, step = 1.0;
    double curr = start;
    double error;
    double best = evaluationError(curr);

    for (int i = 0; i < K_PRECISION - 1; i++) {

        curr = start - step;
        while (curr < end) {
            curr = curr + step;
            error = evaluationError(curr);
            if (error <= best) {
                start = curr;
                best = error;
            }
        }

        start = start - step;
        end = start + step;
        step = step / 10.0;
        std::cout << start << "\n";

    }

    curr = start - step;
    while (curr <= end) {
        curr = curr + step;
        error = evaluationError(curr);
        if (error < best) {
            start = curr;
        }
    }
    std::cout << start << "\n";

    return start;
}

void Tuner::zeroGrad() {
    //gradient.fill({0.0, 0.0});
    for (auto &a: gradient) {
        a[0] = 0.0;
        a[1] = 0.0;
    }
}

void Tuner::calculateGradient() {
    for (auto &p: t_positions) {
        double s = sigmoid(k_param, p.eval);
        double derr = (p.result - s);
        double ds = s * (1 - s);
        double mg_phase = static_cast<double>(p.phase) / TOTAL_PHASE;
        double eg_phase = (TOTAL_PHASE - p.phase) / TOTAL_PHASE;
        // double mg_base = derr * ds * mg_phase;
        // double eg_base = derr * ds * eg_phase;
        for (auto  &t: p.active_coeffs) {
            double temp = (t.wcoef - t.bcoef) * derr * ds;
            //std::cout << temp << "\n";
            gradient[t.index][0] += temp * mg_phase;
            //std::cout << t.wcoef - t.bcoef << "\n";
            gradient[t.index][1] += temp * eg_phase;
        }
    }
}

void Tuner::updateWeights(double lr) {
    for (int i = 0; i < NUM_WEIGHTS / 2; i++) {
        weights[i][0] -= gradient[i][0] * lr;
        weights[i][1] -= gradient[i][1] * lr;
    }  
}

void Tuner::run() {
    k_param = computeOptimalK();
    // std::cout << k_param << "\n";
    // k_param = 0.9;
    for (int epoch = 0; epoch < MAX_EPOCHS; epoch++) {
        std::array<std::array<double, 2>, NUM_WEIGHTS / 2> ada_grad{};

        zeroGrad();
        forward();
        calculateGradient();
        //std::cout << gradient[26][0] << "\n";
        for (int i = 0; i < NUM_WEIGHTS / 2; i++) {
            ada_grad[i][0] += pow(2.0 * gradient[i][0] / NUM_WEIGHTS, 2.0);
            ada_grad[i][1] += pow(2.0 * gradient[i][1] / NUM_WEIGHTS, 2.0);

            weights[i][0] += (k_param * 2.0 / NUM_WEIGHTS) * gradient[i][0] * (LEARNING_RATE / sqrt(1e-8 + ada_grad[i][0]));
            weights[i][1] += (k_param * 2.0 / NUM_WEIGHTS) * gradient[i][1] * (LEARNING_RATE / sqrt(1e-8 + ada_grad[i][1]));
        }

        //updateWeights(LEARNING_RATE);
        std::cout << evaluationError(k_param) << "\n";
    }
}

void Tuner::printWeights() {
    for (int piece = PAWN; piece <= KING; piece++) {
        for (int rank = 0; rank < 8; rank ++) {
            for (int file = 0; file < 8; file++) {
                int sq = rank * 8 + file;
                int idx = piece * 64 + sq;
                std::cout << std::round(weights[idx][0]) + piece_square_tables[piece][0][sq] << ", ";
                // std::cout << weights[idx][0] << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }
}
