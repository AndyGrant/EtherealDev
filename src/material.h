/*
  Ethereal is a UCI chess playing engine authored by Andrew Grant.
  <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>

  Ethereal is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Ethereal is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>

#include "board.h"
#include "types.h"

enum {
    MATERIAL_KEY_SIZE   = 16,
    MATERIAL_SIZE       = 1 << MATERIAL_KEY_SIZE,
    MATERIAL_HASH_SHIFT = 64 - MATERIAL_KEY_SIZE
};

struct MatEntry { uint64_t mhash; int eval; };
struct MatTable { MatEntry entries[MATERIAL_SIZE]; };

extern const uint64_t MaterialPrimes[32];

uint64_t computeMaterialHash(Board *board);
MatEntry* getMaterialEntry(MatTable *mtable, uint64_t mhash);
void storeMaterialEntry(MatTable *mtable, uint64_t mhash, int eval);
