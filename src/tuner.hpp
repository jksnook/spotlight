#pragma once

#include <array>
#include <string>
#include <vector>

/*
Tuning implementation from Andrew Grant's tuning paper:
https://github.com/AndyGrant/Ethereal/blob/master/Tuning.pdf
*/

namespace Spotlight {

const int MAX_POSITIONS = 5000000;
const int NUM_PARAMS = 64 * 6 * 2;
const int PSQ_ARRAY_SIZE = NUM_PARAMS / 2;
const int K_PRECISION = 10;
const int MAX_EPOCHS = 50000;
const int REPORT_INTERVAL = 200;
const double LEARNING_RATE = 0.06;
const std::string TUNING_FILE = "./tune.txt";
const std::string TUNING_PARAMS_FILE = "./piece_squares.txt";

struct CoeffData {
    int wcoef, bcoef;
    int index;
};

struct PositionData {
    int s_eval;
    double d_eval;
    int phase;
    double result;
    std::vector<CoeffData> active_coeffs;
};

class Tuner {
   public:
    Tuner();

    static double sigmoid(double k, double e);

    double staticEvaluationError(double k);
    double evaluationError(double k);
    double computeOptimalK();
    void calculateGradient();
    void zeroGrad();
    void updateWeights(double lr);
    void printWeights();
    void outputToFile();

    void forward();
    void run();
    std::vector<PositionData> t_positions;
    std::array<std::array<double, 2>, PSQ_ARRAY_SIZE> weights;
    std::array<std::array<double, 2>, PSQ_ARRAY_SIZE> gradient;

    double k_param;

   private:
};

}  // namespace Spotlight
