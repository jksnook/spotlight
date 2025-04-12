#pragma once

#include <cstdint>

#define U64 uint64_t
#define move16 uint16_t

enum GenType : int {TT_MOVE, CAPTURES_AND_PROMOTIONS, QUIET, LEGAL, END_MOVEGEN};

enum Color: int {WHITE, BLACK};