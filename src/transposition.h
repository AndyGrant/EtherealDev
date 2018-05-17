/*
  Ethereal is a UCI chess playing engine authored by Andrew Grant.
  <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>
<<<<<<< HEAD
  
=======

>>>>>>> upstream/master
  Ethereal is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
<<<<<<< HEAD
  
=======

>>>>>>> upstream/master
  Ethereal is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
<<<<<<< HEAD
  
=======

>>>>>>> upstream/master
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TRANSPOSITON_H
#define _TRANSPOSITON_H

#include <stdint.h>

#include "types.h"

<<<<<<< HEAD
struct TransEntry {
    int16_t value;
    int8_t depth;
    uint8_t age: 6, type: 2;
    uint16_t bestMove, hash16;
};

struct TransBucket {
    TransEntry entries[BUCKET_SIZE];
};

struct TransTable {
    TransBucket * buckets;
    uint64_t numBuckets;
    uint64_t keySize;
    uint8_t generation;
=======
struct TTEntry {
    int8_t depth;
    uint8_t generation;
    int16_t value;
    uint16_t move;
    uint16_t hash16;
};

struct TTBucket {
    TTEntry slots[4];
};

struct TTable {
    TTBucket *buckets;
    uint8_t generation;
    uint64_t hashMask;
>>>>>>> upstream/master
};

struct PawnKingEntry {
    uint64_t pkhash;
    uint64_t passed;
    int eval;
};

struct PawnKingTable {
    PawnKingEntry entries[0x10000];
};

<<<<<<< HEAD
void initializeTranspositionTable(TransTable* table, uint64_t megabytes);
void destroyTranspositionTable(TransTable* table);
void updateTranspositionTable(TransTable* table);
void clearTranspositionTable(TransTable* table);
int estimateHashfull(TransTable* table);

int getTranspositionEntry(TransTable* table, uint64_t hash, TransEntry* ttEntry);
void storeTranspositionEntry(TransTable* table, int depth, int type, int value, int bestMove, uint64_t hash);

PawnKingEntry * getPawnKingEntry(PawnKingTable* pktable, uint64_t pkhash);
void storePawnKingEntry(PawnKingTable* pktable, uint64_t pkhash, uint64_t passed, int eval);

#endif 
=======
void initTT(uint64_t megabytes);
void updateTT();
void clearTT();
int hashfullTT();
int getTTEntry(uint64_t hash, uint16_t *move, int *value, int *depth, int *bound);
void storeTTEntry(uint64_t hash, uint16_t move, int value, int depth, int bound);

PawnKingEntry* getPawnKingEntry(PawnKingTable *pktable, uint64_t pkhash);
void storePawnKingEntry(PawnKingTable *pktable, uint64_t pkhash, uint64_t passed, int eval);

#endif
>>>>>>> upstream/master
