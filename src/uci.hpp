#pragma once

#include "position.hpp"
#include "movegen.hpp"
#include "search.hpp"
#include "threads.hpp"

#include <sstream>
#include <chrono>
#include <atomic>

namespace Spotlight {

class UCI {
public:
    UCI();

    void loop();
    void parsePosition(std::istringstream& commands);
    void parseGo(std::istringstream& commands);
    void parseSetOption(std::istringstream& commands);

private:
    Position position;
    Threads search_threads;
};

} // namespace Spotlight