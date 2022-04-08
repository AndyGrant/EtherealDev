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
#include <string.h>

#if defined(__linux__)
    #include <sys/mman.h>
#endif

#include "transposition.h"
#include "types.h"

TTable Table; // Global Transposition Table

static int value_from_TT(int value, int height) {

    // When storing MATE/TB scores we factor in the search height
    return value >=  TBWIN_IN_MAX ? value - height
         : value <= -TBWIN_IN_MAX ? value + height : value;
}

static int value_to_TT(int value, int height) {

    // When storing MATE/TB scores we factor in the search height
    return value >=  TBWIN_IN_MAX ? value + height
         : value <= -TBWIN_IN_MAX ? value - height : value;
}


int init_TT(int megabytes) {

    const uint64_t MB = 1ull << 20;
    uint64_t keySize = 16ull;

    // Cleanup memory when resizing the table
    if (Table.hashMask) free(Table.buckets);

    // Default keysize of 16 bits maps to a 2MB TTable
    assert((1ull << 16ull) * sizeof(TTBucket) == 2 * MB);

    // Find the largest keysize that is still within our given megabytes
    while ((1ull << keySize) * sizeof(TTBucket) <= megabytes * MB / 2) keySize++;
    assert((1ull << keySize) * sizeof(TTBucket) <= megabytes * MB);

#if defined(__linux__) && !defined(__ANDROID__)

    // On Linux systems we align on 2MB boundaries and request Huge Pages
    Table.buckets = aligned_alloc(2 * MB, (1ull << keySize) * sizeof(TTBucket));
    madvise(Table.buckets, (1ull << keySize) * sizeof(TTBucket), MADV_HUGEPAGE);
#else

    // Otherwise, we simply allocate as usual and make no requests
    Table.buckets = malloc((1ull << keySize) * sizeof(TTBucket));
#endif

    // Save the lookup mask
    Table.hashMask = (1ull << keySize) - 1u;

    clear_TT(); // Clear the table and load everything into the cache

    // Return the number of MB actually allocated for the TTable
    return ((Table.hashMask + 1) * sizeof(TTBucket)) / MB;
}

void update_TT() {

    // Age is only stored in the upper 6 MSBs
    Table.generation += TT_MASK_BOUND + 1;
}

void clear_TT() {

    // Wipe the Table in preperation for a new game
    memset(Table.buckets, 0, sizeof(TTBucket) * (Table.hashMask + 1u));
}

int hashfullTT() {

    // Take a sample of the first thousand buckets in the table
    // in order to estimate the permill of the table that is in
    // use for the most recent search. We do this, instead of
    // tracking this while probing in order to avoid sharing
    // memory between the search threads.

    int used = 0;

    for (int i = 0; i < 1000; i++)
        for (int j = 0; j < TT_BUCKET_NB; j++)
            used += (Table.buckets[i].slots[j].generation & TT_MASK_BOUND) != BOUND_NONE
                 && (Table.buckets[i].slots[j].generation & TT_MASK_AGE) == Table.generation;

    return used / TT_BUCKET_NB;
}

void prefetchTTEntry(uint64_t hash) {

    TTBucket *bucket = &Table.buckets[hash & Table.hashMask];
    __builtin_prefetch(bucket);
}

int getTTEntry(uint64_t hash, int height, uint16_t *move, int *value, int *eval, int *depth, int *bound) {

    const uint16_t hash16 = hash >> 48;
    TTEntry *slots = Table.buckets[hash & Table.hashMask].slots;

    // Search for a matching hash signature
    for (int i = 0; i < TT_BUCKET_NB; i++) {
        if (slots[i].hash16 == hash16) {

            // Update age but retain bound type
            slots[i].generation = Table.generation | (slots[i].generation & TT_MASK_BOUND);

            // Copy over the TTEntry and signal success
            *move  = slots[i].move;
            *value = value_from_TT(slots[i].value, height);
            *eval  = slots[i].eval;
            *depth = slots[i].depth;
            *bound = slots[i].generation & TT_MASK_BOUND;
            return 1;
        }
    }

    return 0;
}

void storeTTEntry(uint64_t hash, int height, uint16_t move, int value, int eval, int depth, int bound) {

    int i;
    const uint16_t hash16 = hash >> 48;
    TTEntry *slots = Table.buckets[hash & Table.hashMask].slots;
    TTEntry *replace = slots; // &slots[0]

    // Find a matching hash, or replace using MAX(x1, x2, x3),
    // where xN equals the depth minus 4 times the age difference
    for (i = 0; i < TT_BUCKET_NB && slots[i].hash16 != hash16; i++)
        if (   replace->depth - ((259 + Table.generation - replace->generation) & TT_MASK_AGE)
            >= slots[i].depth - ((259 + Table.generation - slots[i].generation) & TT_MASK_AGE))
            replace = &slots[i];

    // Prefer a matching hash, otherwise score a replacement
    replace = (i != TT_BUCKET_NB) ? &slots[i] : replace;

    // Don't overwrite an entry from the same position, unless we have
    // an exact bound or depth that is nearly as good as the old one
    if (   bound != BOUND_EXACT
        && hash16 == replace->hash16
        && depth < replace->depth - 2)
        return;

    // Don't do pure static eval storage when we would overwrite a much
    // deeper search with an actual search score associated with it
    if (    value == VALUE_NONE
        &&  replace->depth > depth + 5
        &&  replace->value != VALUE_NONE
        && (replace->generation & TT_MASK_AGE) == Table.generation)
        return;

    // Finally, copy the new data into the replaced slot
    replace->depth      = (int8_t  ) depth;
    replace->generation = (uint8_t ) bound | Table.generation;
    replace->value      = (int16_t ) value_to_TT(value, height);
    replace->eval       = (int16_t ) eval;
    replace->move       = (uint16_t) move;
    replace->hash16     = (uint16_t) hash16;
}
