#pragma once

#include <atomic>
#include <chrono>
#include <sstream>

#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"
#include "threads.hpp"

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

}  // namespace Spotlight