#pragma once

#include "types.hpp"
#include "utils.hpp"

namespace Spotlight {

extern U64 piece_keys[static_cast<int>(Piece::NO_PIECE) + 1][64];
extern U64 en_passant_keys[64];
extern U64 castle_rights_keys[16];
extern U64 side_key;

void initZobrist();

}  // namespace Spotlight
