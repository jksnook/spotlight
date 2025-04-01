#include "datagen.hpp"

#include <thread>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <string>

bool isQuiet(MoveList &moves) {
    for (const auto &m: moves) {
        if (getMoveType(m) & CAPTURE_MOVE) {
            return false;
        }
    }
    return true;
}

void selfplay(int num_games) {
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.push_back(std::thread(playGames, num_games, i));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].join();
    }
}

void playGames(int num_games, int id) {
    std::ofstream output_file;
    output_file.open("./selfplay" + std::to_string(id) + ".txt");
    if (!output_file.is_open()) return;

    Position pos;

    Search search;
    search.make_output = false;

    std::random_device r;
    std::mt19937 myRandom(r());

    for (int i = 0; i < num_games; i++) {
        std::cout << "starting game " << i << " of " << num_games << " on thread " << id << "\n";
        pos.readFen(STARTPOS);

        std::vector<std::string> fens;

        std::string result;

        int num_random = myRandom() % MAX_RANDOM_MOVES;

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

            pos.makeMove(moves[random_index]);
        }

        while(true) {
            if (pos.fifty_move >= FIFTY_MOVE_LIMIT) {
                std::cout << "Draw by fifty moves rule" << std::endl;
                result = "0.5";
                break;
            } else if (pos.isTripleRepetition()) {
                std::cout << "Draw by triple repetition" << std::endl;
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

            SearchResult search_result = search.nodeSearch(pos, MAX_DEPTH, 100000);

            move16 move = search_result.move;
            int score = search_result.score;

            int eval_score = eval(pos);
            int qscore = search.qScore(pos);

            if (eval_score == qscore && score < MATE_THRESHOLD && score > -MATE_THRESHOLD) {
                fens.push_back(pos.toFen());
            }

            pos.makeMove(move);
        }

        if (output_file.is_open()) {
            for (const auto &f: fens) {
                output_file << f << " " << result << "\n";
            }
        }

    }

    output_file.close();
}