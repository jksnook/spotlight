#pragma once

#include <vector>
#include <array>

const int NUM_WEIGHTS = 64 * 6 * 2;
const int K_PRECISION = 10;
const int MAX_EPOCHS = 5000;
const double LEARNING_RATE = 0.01;

struct WeightData {
    int wcoef, bcoef;
    int index; 
};

struct PositionData {
    int s_eval;
    double eval;
    int phase;
    double result;
    std::vector<WeightData> active_coeffs;
};

class Tuner {
public:
    Tuner();
    
    static double sigmoid(double k, double e);

    double evaluationError(double k);
    double computeOptimalK();
    void calculateGradient();
    void zeroGrad();
    void updateWeights(double lr);
    void printWeights();

    void forward();
    void run();
    std::vector<PositionData> t_positions;
    std::array<std::array<double, 2>, NUM_WEIGHTS / 2> weights;
    std::array<std::array<double, 2>, NUM_WEIGHTS / 2> gradient;

    double k_param;
private:
};