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

#ifndef _TRANSPOSITON_H
#define _TRANSPOSITON_H

#include <stdint.h>

#include "types.h"

struct TransTable {
    TTBucket * buckets;
    uint64_t numBuckets;
    uint64_t keySize;
    uint8_t generation;
};

struct PawnKingEntry {
    uint64_t pkhash;
    uint64_t passed;
    int eval;
};

struct PawnKingTable {
    PawnKingEntry entries[0x10000];
};

void initializeTranspositionTable(uint64_t megabytes);
void destroyTranspositionTable();
void updateTranspositionTable();
void clearTranspositionTable();
int estimateHashfull();

int getTTEntry(uint64_t hash, uint16_t *move, int *value, int *depth, int *bound);
void storeTTEntry(uint64_t hash, uint16_t move, int value, int depth, int bound);

PawnKingEntry * getPawnKingEntry(PawnKingTable* pktable, uint64_t pkhash);
void storePawnKingEntry(PawnKingTable* pktable, uint64_t pkhash, uint64_t passed, int eval);

#endif
