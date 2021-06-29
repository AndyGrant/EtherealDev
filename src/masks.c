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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "attacks.h"
#include "bitboards.h"
#include "masks.h"
#include "types.h"

uint64_t BitsBetweenMasks[SQUARE_NB][SQUARE_NB];
uint64_t AdjacentFilesMasks[FILE_NB];

void initMasks() {

    // Init a table of bitmasks for the squares between two given ones (aligned on diagonal)
    for (int sq1 = 0; sq1 < SQUARE_NB; sq1++)
        for (int sq2 = 0; sq2 < SQUARE_NB; sq2++)
            if (testBit(bishopAttacks(sq1, 0ull), sq2))
                BitsBetweenMasks[sq1][sq2] = bishopAttacks(sq1, 1ull << sq2)
                                           & bishopAttacks(sq2, 1ull << sq1);

    // Init a table of bitmasks for the squares between two given ones (aligned on a straight)
    for (int sq1 = 0; sq1 < SQUARE_NB; sq1++)
        for (int sq2 = 0; sq2 < SQUARE_NB; sq2++)
            if (testBit(rookAttacks(sq1, 0ull), sq2))
                BitsBetweenMasks[sq1][sq2] = rookAttacks(sq1, 1ull << sq2)
                                           & rookAttacks(sq2, 1ull << sq1);

    // Init a table of bitmasks containing the files next to a given file
    for (int file = 0; file < FILE_NB; file++) {
        AdjacentFilesMasks[file]  = Files[MAX(0, file-1)];
        AdjacentFilesMasks[file] |= Files[MIN(FILE_NB-1, file+1)];
        AdjacentFilesMasks[file] &= ~Files[file];
    }
}

uint64_t bitsBetweenMasks(int s1, int s2) {
    assert(0 <= s1 && s1 < SQUARE_NB);
    assert(0 <= s2 && s2 < SQUARE_NB);
    return BitsBetweenMasks[s1][s2];
}

uint64_t adjacentFilesMasks(int file) {
    assert(0 <= file && file < FILE_NB);
    return AdjacentFilesMasks[file];
}