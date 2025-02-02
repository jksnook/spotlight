#include "uci.hpp"

UCI::UCI() {
  Position position;
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
            std::cout << "id name Spotlight" << std::endl;
            std::cout << "uciok" << std::endl;
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
        // printMoveLong(position.parseMove(token));
        position.makeMove(position.parseMove(token));
        // position.print();
    }

}
void UCI::parseGo(std::istringstream& commands) {
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

        U64 search_time;

        if (position.side_to_move == WHITE) {
            search_time = wtime / 30;
        } else {
            search_time = btime / 30;
        }

        move16 best_move = search.iterSearch(position, 30, search_time);
        // printMove(best_move);

        std::cout << "bestmove " << moveToString(best_move) << std::endl;
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

    }
}