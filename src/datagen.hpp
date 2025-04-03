#pragma once

#include "position.hpp"
#include "search.hpp"

#include <fstream>

const int NUM_THREADS = 12;
const int FIFTY_MOVE_LIMIT = 20;
const int MAX_RANDOM_MOVES = 15;
const int MIN_RANDOM_MOVES = 10;

bool isQuiet(MoveList &moves);

void selfplay(int num_games);

void playGames(int num_games, int id);