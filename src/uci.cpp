#include "uci.hpp"
#include "test.hpp"

#include <iostream>
#include <chrono>

namespace Spotlight
{

UCI::UCI(): search_threads(1), position() {
}

void UCI::loop() {
    std::string line;
    std::string token;
    std::istringstream commands;

    while (std::getline(std::cin, line)) {
        commands.clear();
        token.clear();
        commands.str(line);

        commands >> token;

        if (token == "uci") {
            std::cout << "id name Spotlight\n";
            std::cout << "id author github.com/jksnook\n";
            std::cout << "option name Threads type spin default 1 min 1 max 64\n";
            std::cout << "option name Hash type spin default 16 min 1 max 4096\n";
            std::cout << "uciok\n";
        } else if (token == "ucinewgame") {
            search_threads.newGame();
        } else if (token == "isready") {
            std::cout << "readyok" << std::endl;
        } else if (token == "position") {
            //parse the position here
            parsePosition(commands);

        } else if (token == "go") {
            parseGo(commands);
        } else if (token == "quit") {
            break;
        } else if (token == "print") {
            position.print();
            position.printFromBitboard();
            std::cout << position.isTripleRepetition() << std::endl;
            std::cout << position.fifty_move << std::endl;
        } else if (token == "runtest") {
            runTests();
        } else if (token == "setoption") {
            parseSetOption(commands);
        } else if (token == "stop") {
            search_threads.stop();
        }
    }
}

void UCI::parsePosition(std::istringstream& commands) {
    std::string token;
    commands >> token;

    if (token == "startpos") {
        position.readFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
        commands >> token;
    } else if (token == "fen") {
        std::string fen;
        token.clear();
        while(commands >> token) {
            if (token == "moves") {
                break;
            }
            fen += token;
            fen += " ";
            token.clear();
        }
        position.readFen(fen);
    }

    if (token != "moves") {
        return;
    }

    token.clear();

    while (commands >> token) {
        position.makeMove(position.parseMove(token));
        position.game_half_moves++;
    }

}
void UCI::parseGo(std::istringstream& commands) {
    search_threads.stop();
    std::string token;

    commands >> token;
    if (token == "wtime") {
        token.clear();
        commands >> token;
        U64 wtime = stoi(token);
        token.clear();
        commands >> token;
        token.clear();
        commands >> token;
        U64 btime = stoi(token);
        token.clear();
        commands >> token;
        int winc = 0;
        int binc  = 0;
        if (token == "winc") {
            token.clear();
            commands >> token;
            winc = stoi(token);
            token.clear();
            commands >> token;
        }
        if (token == "binc") {
            token.clear();
            commands >> token;
            binc = stoi(token);
            token.clear();
            commands >> token;
        }
        int movestogo = 30;
        if (token == "movestogo") {
            token.clear();
            commands >> token;
            movestogo = stoi(token);
            token.clear();
            commands >> token;
        }
        movestogo = std::min(movestogo, 30);

        U64 search_time;

        if (movestogo == 1) {
            if (position.side_to_move == WHITE) {
                search_time = std::max(wtime - 2ULL, 1ULL);
            } else {
                search_time = std::max(btime - 2ULL, 1ULL);
            }
        } else {
            if (position.side_to_move == WHITE) {
                search_time = wtime / movestogo + winc * 3 / 4;
            } else {
                search_time = btime / movestogo + binc * 3 / 4;
            }
        }

        search_threads.timeSearch(position, search_time);
    } else if (token == "perft") {
        token.clear();
        commands >> token;
        int depth = stoi(token);
        auto start = std::chrono::high_resolution_clock::now();
        U64 node_count = perft(position, depth);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        U64 nps = node_count / duration.count();

        std::cout << node_count << " nodes searched in " << duration.count() << "s " << nps << " nps\n";

    } else if (token == "nodes") {
        token.clear();
        commands >> token;
        U64 num_nodes = stoi(token);

        search_threads.nodeSearch(position, num_nodes);
    } else if (token == "lperft") {
        token.clear();
        commands >> token;
        int depth = stoi(token);
        auto start = std::chrono::high_resolution_clock::now();
        U64 node_count = testLegalPerft(position, depth);
        auto end = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> duration = end - start;
        U64 nps = node_count / duration.count();

        std::cout << node_count << " nodes searched in " << duration.count() << "s " << nps << " nps\n";
    } else if (token == "infinite") {
        search_threads.infiniteSearch(position);
    }
}

void UCI::parseSetOption(std::istringstream& commands) {
    std::string token;
    commands >> token;
    if (token != "name") {
        return;
    }
    token.clear();
    commands >> token;
    if (token == "Threads") {
        token.clear();
        commands >> token;
        if (token != "value") return;
        token.clear();
        commands >> token;
        int num_threads = stoi(token);
        if (num_threads > 64 || num_threads < 1) return;
        search_threads.resize(num_threads);
    } else if (token == "Hash") {
        token.clear();
        commands >> token;
        if (token != "value") return;
        token.clear();
        commands >> token;
        size_t size = stoi(token);
        if (size > 4096 || size < 1) return;
        size *= 1024 * 1024;
        search_threads.tt.resize(size);
    }
}

} // namespace Spotlight
