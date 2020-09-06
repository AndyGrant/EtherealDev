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

#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "material.h"
#include "types.h"

const uint64_t MaterialPrimes[32] = {
    17008651141875982339ull, 11695583624105689831ull, 0ull, 0ull,
    15202887380319082783ull, 13469005675588064321ull, 0ull, 0ull,
    12311744257139811149ull, 15394650811035483107ull, 0ull, 0ull,
    10979190538029446137ull, 18264461213049635989ull, 0ull, 0ull,
    11811845319353239651ull, 15484752644942473553ull, 0ull, 0ull,
};


uint64_t computeMaterialHash(Board *board) {

    uint64_t mhash = 0ull;

    for (int sq = 0; sq < SQUARE_NB; sq++)
        mhash += MaterialPrimes[board->squares[sq]];

    return mhash;
}

MatEntry* getMaterialEntry(MatTable *mtable, uint64_t mhash) {
    MatEntry *mentry = &mtable->entries[mhash >> MATERIAL_HASH_SHIFT];
    return mentry->mhash == mhash ? mentry : NULL;
}

void storeMaterialEntry(MatTable *mtable, uint64_t mhash, int eval) {
    MatEntry *mentry = &mtable->entries[mhash >> MATERIAL_HASH_SHIFT];
    *mentry = (MatEntry) { mhash, eval };
}