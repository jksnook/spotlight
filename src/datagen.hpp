#pragma once

#include "position.hpp"
#include "search.hpp"

#include <fstream>
#include <random>
#include <chrono>

void selfplay(int num_games);

void playGame(std::ofstream &output_file);