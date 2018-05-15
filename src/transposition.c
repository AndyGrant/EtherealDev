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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "move.h"
#include "types.h"
#include "transposition.h"

TransTable TTable;

// Packing of a TTEntry
//  Bits 00-15 : uint16_t hash16
//  Bits 16-21 : uint8_t  age
//  Bits 22-29 : uint8_t  depth
//  Bits 30-31 : uint8_t  bound
//  Bits 32-47 : int16_t  value
//  Bits 48-63 : uint16_t move

static TTEntry packEntry(uint16_t hash, uint8_t age, uint8_t depth, uint8_t bound, uint16_t value, uint16_t move) {
    return ((uint64_t)hash  <<  0) | ((uint64_t)age   << 16)
         | ((uint64_t)depth << 22) | ((uint64_t)bound << 30)
         | ((uint64_t)value << 32) | ((uint64_t)move  << 42);
}

static TTEntry updateAge(TTEntry tte, uint8_t age){
    return (tte & 0xFFFFFFFFFFC0FFFF) | ((uint64_t)age << 16);
}

static uint16_t entryHash(TTEntry tte) {
    return (tte >>  0) & 0xFFFF;
}

static uint8_t entryAge(TTEntry tte) {
    return (tte >> 16) & 0x003F;
}

static uint8_t entryDepth(TTEntry tte) {
    return (tte >> 22) & 0x00FF;
}

static uint8_t entryBound(TTEntry tte) {
    return (tte >> 30) & 0x0003;
}

static int16_t entryValue(TTEntry tte) {
    return (tte >> 32) & 0xFFFF;
}

static uint16_t entryMove(TTEntry tte) {
    return (tte >> 48) & 0xFFFF;
}


void initializeTranspositionTable(uint64_t megabytes) {

    // Minimum table size is 1MB. This maps to a key of size 15.
    // We start at 16, because the loop to adjust the memory
    // size to a power of two ends with a decrement of keySize
    uint64_t keySize = 16ull;

    // Every bucket must be 256 bits for the following scaling
    assert(sizeof(TTBucket) == 32);

    // Scale down the table to the closest power of 2, at or below megabytes
    for (;1ull << (keySize + 5) <= megabytes << 20 ; keySize++);
    keySize -= 1;

    // Setup Table's data members
    TTable.buckets      = malloc((1ull << keySize) * sizeof(TTBucket));
    TTable.numBuckets   = 1ull << keySize;
    TTable.keySize      = keySize;

    clearTranspositionTable();
}

void destroyTranspositionTable() {
    free(TTable.buckets);
}

void updateTranspositionTable() {
    TTable.generation = (TTable.generation + 1) % 64;
}

void clearTranspositionTable() {
    memset(TTable.buckets, 0, sizeof(TTBucket) * TTable.numBuckets);
}

int estimateHashfull() {

    int used = 0;

    for (int i = 0; i < 250 && i < (int64_t)TTable.numBuckets; i++)
        for (int j = 0; j < BUCKET_SIZE; j++)
            used += entryBound(TTable.buckets[i][j]) != BOUND_NONE;

    return 1000 * used / (i * BUCKET_SIZE);
}

int getTTEntry(uint64_t hash, uint16_t *move, int *value, int *depth, int *bound) {

    const uint16_t hash16 = hash >> 48;
    TransBucket bucket = TTable.buckets[hash & (TTable.numBuckets - 1)];

    // Search for a matching hash entry
    for (int i = 0; i < BUCKET_SIZE; i++) {

        if (entryHash(bucket[i]) == hash16) {

            // Update the age of the entry
            const TTEntry tte = bucket[i];
            bucket[i] = updateAge(tte, TTable.generation);

            // Unpack the data for the search
            *move  = entryMove(tte);
            *value = entryValue(tte);
            *depth = entryDepth(tte);
            *bound = entryBound(tte);

            return 1;
        }
    }

    return 0;
}

void storeTTEntry(uint64_t hash, uint16_t move, int value, int depth, int bound) {

    assert(abs(value) <= MATE);
    assert(0 <= depth && depth <= MAX_PLY);
    assert(bound == BOUND_EXACT || bound == BOUND_LOWER || bound == BOUND_UPPER);

    const uint16_t hash16 = hash >> 48;
    TTEntry *outdated = NULL, *lowdraft = NULL, *replace = NULL;
    TransBucket bucket = TTable.buckets[hash & (TTable.numBuckets - 1)];

    for (int i = 0; i < BUCKET_SIZE; i++) {

        // Found an unused or hash matching entry
        if (entryBound(bucket[i]) == BOUND_NONE || entryHash(bucket[i]) == hash16){
            replace = &bucket[i];
            break;
        }

        // Search for the lowest draft of an older entry
        if (    entryAge(bucket[i]) != TTable.generation
            && (outdated == NULL || entryDepth(*outdated) >= entryDepth(bucket[i])))
            outdated = &bucket[i];

        // Search for the lowest draft when there are no older entries
        if (    outdated == NULL
            && (lowdraft == NULL || entryDepth(*lowdraft) >= entryDepth(bucket[i])))
            lowdraft = &bucket[i];
    }

    // If we have to throw out an old entry, choose the
    // older option before considering a current option
    replace = replace  != NULL ? replace
            : outdated != NULL ? outdated : lowdraft;

    // Don't throw out an entry unless the new one is nearly as
    // good, or is the EXACT bound for a search, as per Stockfish
    if (    bound == BOUND_EXACT
        ||  hash16 != entryHash(*replace)
        ||  depth >= entryDepth(*replace) - 3)
        *replace = packEntry(hash16, TTable.generation, depth, bound, value, move);
}

PawnKingEntry * getPawnKingEntry(PawnKingTable* pktable, uint64_t pkhash){
    PawnKingEntry* pkentry = &(pktable->entries[pkhash >> 48]);
    return pkentry->pkhash == pkhash ? pkentry : NULL;
}

void storePawnKingEntry(PawnKingTable* pktable, uint64_t pkhash, uint64_t passed, int eval){
    PawnKingEntry* pkentry = &(pktable->entries[pkhash >> 48]);
    pkentry->pkhash = pkhash;
    pkentry->passed = passed;
    pkentry->eval   = eval;
}
