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

TTable Table; // Global Transposition Table

const int FiftyOffset[3] = { 0, 5, 10 };

const uint16_t FiftyMask[3] = { 0x7FE0, 0x7C1F, 0x03FF };

void initTT(uint64_t megabytes) {

    // Free up memory if we already allocated
    if (Table.hashMask != 0ull) free(Table.buckets);

    // We set the smallest TT to 1 MB. This is a TT with a lookup
    // key with 15 bits. We start with 16 bits, because the scaling
    // ends by decrementing the key size by 1 bit.
    uint64_t keySize = 16ull;

    // Buckets must be 32 bytes for the scaling and for alignment
    assert(sizeof(TTBucket) == 32);

    // Scale down the table to the closest power of 2, at or below megabytes
    for (;1ull << (keySize + 5) <= megabytes << 20 ; keySize++);
    keySize -= 1;

    // Allocate all of our TTBuckets and TTEntries
    Table.buckets = malloc((1ull << keySize) * sizeof(TTBucket));

    // We lookup the table with the lowest keySize bits of a hash
    Table.hashMask   = (1ull << keySize) - 1u;

    clearTT(); // Reset the TT for a new search
}

void updateTT() {
    Table.age += 0x04u; // Lower two bits for bound type
    Table.age %= 0x40u; // Upper two bits for fifty counter
    assert(!(Table.age & 0xC3u)); // Ensure clear bits
}

void clearTT() {
    memset(Table.buckets, 0, sizeof(TTBucket) * (Table.hashMask + 1u));
}

int hashfullTT() {

    int used = 0;

    // Sample the first 3,000 slots of the table
    for (int i = 0; i < 1000; i++)
        for (int j = 0; j < 3; j++)
            used += (Table.buckets[i].slots[j].data & 0x0C) != 0x00
                 && (Table.buckets[i].slots[j].data & 0x3C) == Table.age;

    return used / 3;
}

int getTTEntry(uint64_t hash, uint16_t *move, int *value, int *eval, int *depth, int *bound, int *fifty) {

    uint8_t partial;
    const uint16_t hash16 = hash >> 48;
    TTBucket *bucket = &Table.buckets[hash & Table.hashMask];
    TTEntry *slots = &bucket->slots[0];

    // Search for a matching hash signature
    for (int i = 0; i < 3; i++) {

        if (slots[i].hash16 == hash16) {

            // Update age, retain the bounds and fifty bits
            slots[i].data = Table.age | (slots[i].data & 0xC3);

            // Grab upper bits of fifty move rule before copying over
            partial = ((bucket->fifty & ~FiftyMask[i]) >> FiftyOffset[i]) << 2;

            // Copy over the TTEntry and signal success
            *move  = slots[i].move;
            *value = slots[i].value;
            *eval  = slots[i].eval;
            *depth = slots[i].depth;
            *bound = slots[i].data & 0x3;
            *fifty = (slots[i].data >> 6) | partial;
            return 1;
        }
    }

    return 0; // No TTEntry found
}

void storeTTEntry(uint64_t hash, uint16_t move, int value, int eval, int depth, int bound, int fifty) {

    assert(abs(value) <= MATE);
    assert(abs(eval) <= MATE || eval == VALUE_NONE);
    assert(0 <= depth && depth < MAX_PLY);
    assert(bound == BOUND_LOWER || bound == BOUND_UPPER || bound == BOUND_EXACT);
    assert(0 <= fifty && fifty <= 100);

    int idx;
    const uint16_t hash16 = hash >> 48;
    TTBucket *bucket = &Table.buckets[hash & Table.hashMask];
    TTEntry *replace = NULL, *slots = &bucket->slots[0];

    for (int i = 0; i < 3; i++) {

        // Found a matching hash or an unused entry
        if (slots[i].hash16 == hash16 || !(slots[i].data & 0x3)) {
            replace = &slots[i];
            idx = i;
            break;
         }

        // Take the first entry as a starting point
        if (i == 0) {
            replace = &slots[i];
            idx = i;
            continue;
        }

        // Replace using MAX(x1, x2), where xN = depth - 4 * age difference
        if (   replace->depth - ((259 + (Table.age & 0x3F) - (replace->data & 0x3F)) & 0xFC)
            >= slots[i].depth - ((259 + (Table.age & 0x3F) - (slots[i].data & 0x3F)) & 0xFC))
            replace = &slots[i], idx = i;
    }

    // Don't overwrite an entry from the same position, unless we have
    // an exact bound or depth that is nearly as good as the old one
    if (    bound != BOUND_EXACT
        &&  hash16 == replace->hash16
        &&  depth < replace->depth - 3)
        return;

    // Save new data into the replaced slot
    replace->depth  = (int8_t)depth;
    replace->data   = (uint8_t)bound | Table.age;
    replace->value  = (int16_t)value;
    replace->eval   = (int16_t)eval;
    replace->move   = (uint16_t)move;
    replace->hash16 = (uint16_t)hash16;

    // Store the fifty move counter. We make use of the upper two bits
    // from replace->data, as well as five bits from the bucket's fifty
    replace->data |= ((uint8_t)fifty << 6) & 0xFF;
    bucket->fifty &= FiftyMask[idx];
    bucket->fifty |= ((uint8_t)fifty >> 2) << FiftyOffset[idx];
}

PawnKingEntry* getPawnKingEntry(PawnKingTable *pktable, uint64_t pkhash) {
    PawnKingEntry *pkentry = &pktable->entries[pkhash >> 48];
    return pkentry->pkhash == pkhash ? pkentry : NULL;
}

void storePawnKingEntry(PawnKingTable *pktable, uint64_t pkhash, uint64_t passed, int eval) {
    PawnKingEntry *pkentry = &pktable->entries[pkhash >> 48];
    pkentry->pkhash = pkhash;
    pkentry->passed = passed;
    pkentry->eval   = eval;
}
