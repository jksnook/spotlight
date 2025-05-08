#pragma once

#include "position.hpp"
#include "search.hpp"

#include <fstream>
#include <mutex>

namespace Spotlight
{

const int NUM_THREADS = 12;
const int FIFTY_MOVE_LIMIT = 20;
const int MAX_RANDOM_MOVES = 15;
const int MIN_RANDOM_MOVES = 5;
const int BASE_NODE_COUNT = 5000;

bool isQuiet(MoveList &moves);

void selfplay(int num_games);

void playGames(int num_games, int id, int &games_played, std::mutex &mx);

} // namespace Spotlight
