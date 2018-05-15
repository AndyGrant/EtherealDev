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

// typdef uint64_t TTEntry:
//  Bits  0 - 15 : uint16 move
//  Bits 16 - 31 :  int16 eval
//  Bits 32 - 47 :  int16 value
//  Bits 48 - 55 :  uint8 depth
//  Bits 56 - 57 :  uint2 bound
//  Bits 58 - 63 :  uint6 age

// For TTBuckets:
//  Bits   0 -  63 : TTEntry 1
//  Bits  64 - 127 : TTEntry 2
//  Bits 128 - 191 : TTEntry 3
//  Bits 192 - 207 : uint16 Hash1
//  Bits 208 - 223 : uint16 Hash2
//  Bits 224 - 239 : uint16 Hash3
//  Bits 240 - 255 : uint16 Padding

struct TTBucket {
    TTEntry entries[3];
    uint16_t hashes[4];
};

struct TTable {
    TTBucket *buckets;
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

void initTT(uint64_t megabytes);
void destroyTT();
void updateTT();
void clearTT();
int hashfullTT();

int getTTEntry(uint64_t hash, uint16_t *move, int *eval, int *value, int *depth, int *bound);
void storeTTEntry(uint64_t hash, uint16_t move, int eval, int value, int depth, int bound);

PawnKingEntry * getPawnKingEntry(PawnKingTable* pktable, uint64_t pkhash);
void storePawnKingEntry(PawnKingTable* pktable, uint64_t pkhash, uint64_t passed, int eval);

#endif
