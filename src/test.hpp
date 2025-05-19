#pragma once

#include <array>
#include <string>
#include <string_view>
#include <vector>

#include "position.hpp"
#include "types.hpp"

namespace Spotlight {

void runTests();

void testSee();

void testCheck();

void testPerft();

void testSearch();

void testMovePicker();

U64 testLegalPerft(Position &pos, int depth);

U64 testLegalPerftHelper(Position &pos, int depth);

void testMoveVerification();

}  // namespace Spotlight
