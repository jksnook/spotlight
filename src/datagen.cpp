#include "datagen.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>

namespace Spotlight {

bool isQuiet(MoveList& moves) {
    for (const auto& m : moves) {
        if (getMoveType(m.move) & CAPTURE_MOVE) {
            return false;
        }
    }
    return true;
}

void selfplay(int num_games, int num_threads, U64 node_count) {
    std::vector<std::thread> threads;
    std::mutex mx;
    int games_played = 0;

    for (int i = 0; i < num_threads; i++) {
        threads.push_back(
            std::thread(playGames, num_games, node_count, i, std::ref(games_played), std::ref(mx)));
    }

    for (int i = 0; i < num_threads; i++) {
        threads[i].join();
    }
}

void playGames(int num_games, U64 node_count, int id, int& games_played, std::mutex& mx) {
    std::ofstream output_file;
    output_file.open("./selfplay" + std::to_string(id) + ".txt");
    if (!output_file.is_open()) return;

    Position pos;

    TT tt;
    std::atomic<bool> is_stopped(false);

    Search search(&tt, &is_stopped, [&]() { return search.nodes_searched; });
    search.make_output = false;

    std::random_device r;
    std::mt19937 myRandom(r());

    for (int i = 0; i < num_games; i++) {
        mx.lock();
        games_played++;
        if (games_played > num_games) {
            mx.unlock();
            break;
        };
        std::cout << "starting game " << games_played << " of " << num_games << " on thread " << id
                  << " with the following position and number of random moves\n";
        mx.unlock();
        pos = Position();
        tt.clear();
        search.clearHistory();

        std::vector<std::string> fens;

        std::string result;

        int num_random =
            (myRandom() % (MAX_RANDOM_MOVES - MIN_RANDOM_MOVES + 1)) + MIN_RANDOM_MOVES;
        std::cout << num_random << "\n";

        // make some random moves
        for (int i = 0; i < num_random; i++) {
            if (pos.isTripleRepetition()) {
                std::cout << "Draw" << std::endl;
                result = "0.5";
                break;
            }

            MoveList moves;

            if (pos.side_to_move == WHITE) {
                generateMovesSided<WHITE, LEGAL>(moves, pos);
                if (moves.size() == 0) {
                    if (inCheckSided<WHITE>(pos)) {
                        std::cout << "Black wins" << std::endl;
                        result = "0";
                    } else {
                        std::cout << "Stalemate" << std::endl;
                        result = "0.5";
                    }
                    break;
                }
            } else {
                generateMovesSided<BLACK, LEGAL>(moves, pos);
                if (moves.size() == 0) {
                    if (inCheckSided<BLACK>(pos)) {
                        std::cout << "White wins" << std::endl;
                        result = "1";
                    } else {
                        std::cout << "Stalemate" << std::endl;
                        result = "0.5";
                    }
                    break;
                }
            }

            int random_index = myRandom() % moves.size();

            pos.makeMove(moves[random_index].move);
        }
        pos.print();

        while (true) {
            if (pos.fifty_move >= FIFTY_MOVE_LIMIT) {
                std::cout << "Draw by fifty moves rule\n";
                pos.print();
                result = "0.5";
                break;
            } else if (pos.isTripleRepetition()) {
                std::cout << "Draw by triple repetition\n";
                pos.print();
                result = "0.5";
                break;
            } else if (countBits(pos.bitboards[OCCUPANCY]) == 2) {
                std::cout << "Draw by insufficient material\n";
                pos.print();
                result = "0.5";
                break;
            }

            MoveList moves;

            if (pos.side_to_move == WHITE) {
                generateMovesSided<WHITE, LEGAL>(moves, pos);
                if (moves.size() == 0) {
                    if (inCheckSided<WHITE>(pos)) {
                        std::cout << "Black wins" << std::endl;
                        result = "0";
                    } else {
                        std::cout << "Stalemate" << std::endl;
                        result = "0.5";
                    }
                    break;
                }
            } else {
                generateMovesSided<BLACK, LEGAL>(moves, pos);
                if (moves.size() == 0) {
                    if (inCheckSided<BLACK>(pos)) {
                        std::cout << "White wins" << std::endl;
                        result = "1";
                    } else {
                        std::cout << "Stalemate" << std::endl;
                        result = "0.5";
                    }
                    break;
                }
            }

            is_stopped.store(false);
            SearchResult search_result =
                search.nodeSearch(pos, MAX_PLY, node_count + myRandom() % (node_count / 10));

            move16 move = search_result.move;
            int score = search_result.score;

            int eval_score = eval(pos);
            int qscore = search.qScore(pos);

            if (eval_score == qscore && score < MATE_THRESHOLD && score > -MATE_THRESHOLD) {
                fens.push_back(pos.toFen());
            }

            pos.makeMove(move);
            tt.nextGeneration();
        }

        if (output_file.is_open()) {
            // log a maximum of about 10 positions from each game to the output file
            for (unsigned int f = 0; f < fens.size(); f += std::max(static_cast<int>(fens.size() / 10), 1)) {
                output_file << fens[f] << " " << result << "\n";
            }
        }
    }

    output_file.close();
}

}  // namespace Spotlight
