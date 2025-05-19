#pragma once

#include <fstream>
#include <mutex>

#include "position.hpp"
#include "search.hpp"

namespace Spotlight {

const int NUM_THREADS = 12;
const int FIFTY_MOVE_LIMIT = 20;
const int MAX_RANDOM_MOVES = 15;
const int MIN_RANDOM_MOVES = 5;
const int BASE_NODE_COUNT = 5000;

bool isQuiet(MoveList &moves);

void selfplay(int num_games, int num_threads, U64 node_count);

void playGames(int num_games, U64 node_count, int id, int &games_played, std::mutex &mx);

}  // namespace Spotlight
