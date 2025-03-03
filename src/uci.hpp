#pragma once

#include "position.hpp"
#include "movegen.hpp"
#include "search.hpp"

#include <sstream>
#include <chrono>


class UCI {
public:
  UCI();

  void loop();

  void parsePosition(std::istringstream& commands);
  void parseGo(std::istringstream& commands);

private:
  Position position;

  Search search;

};