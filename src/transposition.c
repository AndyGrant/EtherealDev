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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "move.h"
#include "types.h"
#include "transposition.h"


TTable Table; // Global TTable used by all threads

static uint16_t entryMove(TTEntry tte) {
    return tte & 0xFFFF;
}

static int entryEval(TTEntry tte) {
    return (int16_t)((tte >> 16) & 0xFFFF);
}

static int entryValue(TTEntry tte) {
    return (int16_t)((tte >> 32) & 0xFFFF);
}

static int entryDepth(TTEntry tte) {
    return (uint8_t)((tte >> 48) & 0xFF);
}

static int entryBound(TTEntry tte) {
    return (uint8_t)((tte >> 56) & 0x3);
}

static uint8_t entryAge(TTEntry tte) {
    return (uint8_t)((tte >> 58) & 0x3F);
}

static TTEntry entryUpdateAge(TTEntry tte) {
    return (tte & 0xC0FFFFFFFFFFFFFF) | (uint64_t)Table.generation;
}

static TTEntry entryPack(uint16_t move, int eval, int value, int depth, int bound) {
    TTEntry tte = 0ull;
    tte |= Table.generation; tte <<=  2;
    tte |= (uint8_t )bound;  tte <<=  8;
    tte |= (uint8_t )depth;  tte <<= 16;
    tte |= (uint16_t)value;  tte <<= 16;
    tte |= (uint16_t)eval;   tte <<= 16;
    tte |= move;             tte <<=  0;
    return tte;
}

static void entryUnpack(TTEntry tte, uint16_t *move, int *eval, int *value, int *depth, int *bound) {
    *move  = entryMove(tte);
    *eval  = entryEval(tte);
    *value = entryValue(tte);
    *depth = entryDepth(tte);
    *bound = entryBound(tte);
}


void initTT(uint64_t megabytes) {

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
    Table.buckets    = malloc((1ull << keySize) * sizeof(TTBucket));
    Table.numBuckets = 1ull << keySize;
    Table.keySize    = keySize;

    clearTT();
}

void destroyTT() {
    free(Table.buckets);
}

void updateTT() {
    Table.generation = (Table.generation + 1) % 64;
}

void clearTT() {
    memset(Table.buckets, 0, sizeof(TTBucket) * Table.numBuckets);
}

int hashfullTT() {

    int used = 0;

    for (int i = 0; i < 250; i++)
        for (int j = 0; j < 3; j++)
            used += entryBound(Table.buckets[i].entries[j]) != BOUND_NONE;

    return 1000 * used / (250 * 3);
}

int getTTEntry(uint64_t hash, uint16_t *move, int *eval, int *value, int *depth, int *bound) {

    const uint16_t hash16 = hash >> 48;
    TTBucket *bucket = &Table.buckets[hash & (Table.numBuckets - 1)];



    // Search for an entry with a matching hash16
    for (int i = 0; i < 3; i++) {
        if (hash16 == bucket->hashes[i]) {

            // Save off the TTEntry as soon as possible
            const uint64_t ttEntry = bucket->entries[i];

            // Refresh the entry age to match the current search
            bucket->entries[i] = entryUpdateAge(ttEntry);

            // Set each of the terms used by the search
            entryUnpack(ttEntry, move, eval, value, depth, bound);
            return 1;
        }
    }

    return 0;
}

void storeTTEntry(uint64_t hash, uint16_t move, int eval, int value, int depth, int bound) {

    assert(abs(value) <= MATE);
    assert(0 <= depth && depth < MAX_PLY);
    assert(bound == BOUND_LOWER || bound == BOUND_UPPER || bound == BOUND_EXACT);

    int idx;
    const uint16_t hash16 = hash >> 48;

    TTEntry *oldest = NULL, *lowest = NULL, *replace = NULL;
    TTBucket *bucket = &Table.buckets[hash & (Table.numBuckets - 1)];

    // Search the bucket for a candidate to replace
    for (int i = 0; i < 3; i++) {

        uint64_t candidate = bucket->entries[i];

        // Found a matching entry or an used one
        if (   hash16 == bucket->hashes[i]
            || entryBound(candidate) == BOUND_NONE) {
            replace = &bucket->entries[i];
            break;
        }

        // Search for the lowest draft of an old entry
        if (    Table.generation != entryAge(candidate)
            && (oldest == NULL || entryDepth(*oldest) >= entryDepth(candidate)))
            oldest = &bucket->entries[i];

        // Search for the lowest draft of any entry
        if (    oldest == NULL
            && (lowest == NULL || entryDepth(*lowest) >= entryDepth(candidate)))
            lowest = &bucket->entries[i];
    }

    // If no matching or unused entry, take the oldest and then the lowest draft
    replace = replace != NULL ? replace
            :  oldest != NULL ?  oldest : lowest;

    idx = (int)(replace - bucket->entries);

    // Don't overwrite an entry from the same position when the new depth
    // is much lower than the entry depth, and the new bound is not exact
    if (    bound != BOUND_EXACT
        &&  depth < entryDepth(*replace) - 3
        &&  hash16 == bucket->hashes[idx])
        return;

    // Replace after packing the data into a TTEntry (uint64_t)
    *replace = entryPack(move, eval, value, depth, bound);
    bucket->hashes[idx] = hash16;

    assert(entryMove(*replace) == move);
    assert(entryEval(*replace) == eval);
    assert(entryValue(*replace) == value);
    assert(entryDepth(*replace) == depth);
    assert(entryBound(*replace) == bound);
    assert(entryAge(*replace) == Table.generation);
    assert(hash16 == bucket->hashes[idx]);
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
