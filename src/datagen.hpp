#pragma once

#include "position.hpp"
#include "search.hpp"

#include <fstream>

const int NUM_THREADS = 12;
const int FIFTY_MOVE_LIMIT = 20;
const int MAX_RANDOM_MOVES = 16;

bool isQuiet(MoveList &moves);

void selfplay(int num_games);

void playGames(int num_games, int id);