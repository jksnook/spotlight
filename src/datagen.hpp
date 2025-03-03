#pragma once

#include "position.hpp"
#include "search.hpp"

#include <fstream>

const int NUM_THREADS = 12;

void selfplay(int num_games);

void playGames(int num_games, int id);