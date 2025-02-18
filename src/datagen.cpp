#include "datagen.hpp"

#include <thread>
#include <vector>
#include <array>

void selfplay(int num_games) {
    std::ofstream output_file;

    output_file.open("selfplay.txt");

    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.push_back(std::thread(playGame, std::ref(output_file)));
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i].join();
    }

    output_file.close();
}

void playGame(std::ofstream &output_file) {
    Position pos;

    Search search;

    std::random_device r;
    std::mt19937 myRandom(r());

    std::vector<std::string> fens;

    std::string result;

    // make some random moves
    for (int i = 0; i < 10; i++) {
        if (pos.isTripleRepetition()) {
            std::cout << "Draw" << std::endl;
            result = "0.5";
            break;
        }

        MoveList moves;

        if (pos.side_to_move == WHITE) {
            generateMoves<true>(moves, pos);
            if (moves.size() == 0) {
                if (inCheck<true>(pos)) {
                    std::cout << "Black wins" << std::endl;
                    result = "0";
                } else {
                    std::cout << "Stalemate" << std::endl;
                    result = "0.5";
                }
                break;
            }
        } else {
            generateMoves<false>(moves, pos);
            if (moves.size() == 0) {
                if (inCheck<false>(pos)) {
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

    int score = 0;

    while(true) {
        if (pos.fifty_move >= 50) {
            std::cout << "Draw by fifty moves rule" << std::endl;
            result = "0.5";
            break;
        } else if (pos.isTripleRepetition()) {
            std::cout << "Draw by triple repetition" << std::endl;
            result = "0.5";
            break;
        }

        if (score < MATE_THRESHOLD && score > -MATE_THRESHOLD) {
            fens.push_back(pos.toFen());
        }

        MoveList moves;

        if (pos.side_to_move == WHITE) {
            generateMoves<true>(moves, pos);
            if (moves.size() == 0) {
                if (inCheck<true>(pos)) {
                    std::cout << "Black wins" << std::endl;
                    result = "0";
                } else {
                    std::cout << "Stalemate" << std::endl;
                    result = "0.5";
                }
                break;
            }
        } else {
            generateMoves<false>(moves, pos);
            if (moves.size() == 0) {
                if (inCheck<false>(pos)) {
                    std::cout << "White wins" << std::endl;
                    result = "1";
                } else {
                    std::cout << "Stalemate" << std::endl;
                    result = "0.5";
                }
                break;
            }
        }

        SearchResult search_result = search.iterSearch(pos, MAX_DEPTH, 150);

        move16 move = search_result.move;
        score = search_result.score;

        pos.makeMove(move);
    }

    if (output_file.is_open()) {
        for (const auto &f: fens) {
            output_file << f << " " << result << "\n";
        }
    }
}