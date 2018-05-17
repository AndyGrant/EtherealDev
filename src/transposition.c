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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>

#include "move.h"
#include "types.h"
#include "transposition.h"

<<<<<<< HEAD
TransTable Table;

void initializeTranspositionTable(TransTable* table, uint64_t megabytes){
    
    // Minimum table size is 1MB. This maps to a key of size 15.
    // We start at 16, because the loop to adjust the memory
    // size to a power of two ends with a decrement of keySize
    uint64_t keySize = 16ull;
    
    // Every bucket must be 256 bits for the following scaling
    assert(sizeof(TransBucket) == 32);
=======
TTable Table; // Global Transposition Table

void initTT(uint64_t megabytes) {

    // Free up memory if we already allocated
    if (Table.hashMask != 0ull) free(Table.buckets);

    // We set the smallest TT to 1 MB. This is a TT with a lookup
    // key with 15 bits. We start with 16 bits, because the scaling
    // ends by decrementing the key size by 1 bit.
    uint64_t keySize = 16ull;

    // Buckets must be 32 bytes for the scaling and for alignment
    assert(sizeof(TTBucket) == 32);
>>>>>>> upstream/master

    // Scale down the table to the closest power of 2, at or below megabytes
    for (;1ull << (keySize + 5) <= megabytes << 20 ; keySize++);
    keySize -= 1;
<<<<<<< HEAD
    
    // Setup Table's data members
    table->buckets      = malloc((1ull << keySize) * sizeof(TransBucket));
    table->numBuckets   = 1ull << keySize;
    table->keySize      = keySize;

    clearTranspositionTable(table);
}

void destroyTranspositionTable(TransTable* table){
    free(table->buckets);
}

void updateTranspositionTable(TransTable* table){
    table->generation = (table->generation + 1) % 64;
}

void clearTranspositionTable(TransTable* table){

    table->generation = 0u;

    memset(table->buckets, 0, sizeof(TransBucket) * table->numBuckets);
    
}

int estimateHashfull(TransTable* table){
    
    int i, used = 0;
    
    for (i = 0; i < 250 && i < (int64_t)table->numBuckets; i++)
        used += (table->buckets[i].entries[0].type != 0)
             +  (table->buckets[i].entries[1].type != 0)
             +  (table->buckets[i].entries[2].type != 0)
             +  (table->buckets[i].entries[3].type != 0);
             
    return 1000 * used / (i * 4);
}

int getTranspositionEntry(TransTable* table, uint64_t hash, TransEntry* ttEntry){
    
    TransBucket* bucket = &(table->buckets[hash & (table->numBuckets - 1)]);
    int i; uint16_t hash16 = hash >> 48;
    
    #ifdef TEXEL
    return NULL;
    #endif
    
    // Search for a matching entry. Update the generation if found.
    for (i = 0; i < BUCKET_SIZE; i++){
        if (bucket->entries[i].hash16 == hash16){
            bucket->entries[i].age = table->generation;
            memcpy(ttEntry, &bucket->entries[i], sizeof(TransEntry));
            return 1;
        }
    }
    
    return 0;
}

void storeTranspositionEntry(TransTable* table, int depth, int type, int value, int bestMove, uint64_t hash){
    
    // Validate Parameters
    assert(depth < MAX_PLY && depth >= 0);
    assert(type == PVNODE || type == CUTNODE || type == ALLNODE);
    assert(value <= MATE && value >= -MATE);
    
    TransEntry* entries = table->buckets[hash & (table->numBuckets - 1)].entries;
    TransEntry* oldOption = NULL;
    TransEntry* lowDraftOption = NULL;
    TransEntry* toReplace = NULL;
    
    int i; uint16_t hash16 = hash >> 48;
    
    for (i = 0; i < BUCKET_SIZE; i++){
        
        // Found an unused entry
        if (entries[i].type == 0){
            toReplace = &(entries[i]);
            break;
        }
        
        // Found an entry with the same hash
        if (entries[i].hash16 == hash16){
            toReplace = &(entries[i]);
            break;
        }
        
        // Search for the lowest draft of an old entry
        if (    entries[i].age != table->generation
            && (oldOption == NULL || oldOption->depth >= entries[i].depth))
            oldOption = &(entries[i]);
        
        // Search for the lowest draft if no old entry has been found yet
        if (    oldOption == NULL
            && (lowDraftOption == NULL || lowDraftOption->depth >= entries[i].depth))
            lowDraftOption = &(entries[i]);
    }
    
    // If we found an empty slot or matching hash we will replace that,
    // otherwise we look an entry which came from a different generation,
    // and finally we replace the lowest depth entry in the bucket
    toReplace = toReplace != NULL ? toReplace 
              : oldOption != NULL ? oldOption 
              : lowDraftOption;
    
    // We will not overwrite an entry from the same position, unless
    // the search depth is near the entry depth, or if the entry we
    // are saving is an exact bound. This is taken from Stockfish.
    if (    type == PVNODE
        ||  hash16 != toReplace->hash16
        ||  depth >= toReplace->depth - 3){
        
        toReplace->value    = value;
        toReplace->depth    = depth;
        toReplace->age      = table->generation;
        toReplace->type     = type;
        toReplace->bestMove = bestMove;
        toReplace->hash16   = hash16;
    }
}

PawnKingEntry * getPawnKingEntry(PawnKingTable* pktable, uint64_t pkhash){
    PawnKingEntry* pkentry = &(pktable->entries[pkhash >> 48]);
    return pkentry->pkhash == pkhash ? pkentry : NULL;
}

void storePawnKingEntry(PawnKingTable* pktable, uint64_t pkhash, uint64_t passed, int eval){
    PawnKingEntry* pkentry = &(pktable->entries[pkhash >> 48]);
=======

    // Allocate all of our TTBuckets and TTEntries
    Table.buckets = malloc((1ull << keySize) * sizeof(TTBucket));

    // We lookup the table with the lowest keySize bits of a hash
    Table.hashMask   = (1ull << keySize) - 1u;

    clearTT(); // Reset the TT for a new search
}

void updateTT() {
    Table.generation += 4; // Pad lower bits for bounds
}

void clearTT() {
    memset(Table.buckets, 0, sizeof(TTBucket) * (Table.hashMask + 1u));
}

int hashfullTT() {

    int used = 0;

    // Sample the first 1,000 slots of the table
    for (int i = 0; i < 250; i++)
        for (int j = 0; j < 4; j++)
            used += (Table.buckets[i].slots[j].generation & 0x0C) != 0x00
                 && (Table.buckets[i].slots[j].generation & 0xFC) == Table.generation;

    return used;
}

int getTTEntry(uint64_t hash, uint16_t *move, int *value, int *depth, int *bound) {

    const uint16_t hash16 = hash >> 48;
    TTEntry *slots = &Table.buckets[hash & Table.hashMask].slots[0];

    // Search for a matching hash signature
    for (int i = 0; i < 4; i++) {

        if (slots[i].hash16 == hash16) {

            // Update age, retain the bounds stored in the lower two bits
            slots[i].generation = Table.generation | (slots[i].generation & 0x3);

            // Copy over the TTEntry and signal success
            *move  = slots[i].move;
            *value = slots[i].value;
            *depth = slots[i].depth;
            *bound = slots[i].generation & 0x3;
            return 1;
        }
    }

    return 0; // No TTEntry found
}

void storeTTEntry(uint64_t hash, uint16_t move, int value, int depth, int bound) {

    assert(abs(value) <= MATE); // Scores over MATE may not be converted correctly
    assert(0 <= depth && depth < MAX_PLY); // Depth should be within our regular search
    assert(bound == BOUND_LOWER || bound == BOUND_UPPER || bound == BOUND_EXACT);

    const uint16_t hash16 = hash >> 48;
    TTEntry *replace = NULL, *slots = &Table.buckets[hash & Table.hashMask].slots[0];

    for (int i = 0; i < 4; i++) {

        // Found a matching hash or an unused entry
        if (slots[i].hash16 == hash16 || (slots[i].generation & 0x3) == 0u) {
            replace = &slots[i];
            break;
         }

        // Take the first entry as a starting point
        if (i == 0) {
            replace = &slots[i];
            continue;
        }

        // Replace using MAX(x1, x2), where xN = depth - 8 * age difference
        if (   replace->depth - ((259 + Table.generation - replace->generation) & 0xFC) * 2
            >= slots[i].depth - ((259 + Table.generation - slots[i].generation) & 0xFC) * 2)
            replace = &slots[i];
    }

    // Don't overwrite an entry from the same position, unless we have
    // an exact bound or depth that is nearly as good as the old one
    if (    bound != BOUND_EXACT
        &&  hash16 == replace->hash16
        &&  depth < replace->depth - 3)
        return;

    // Finally, copy the new data into the replaced slot
    replace->depth      = (int8_t)depth;
    replace->generation = (uint8_t)bound | Table.generation;
    replace->value      = (int16_t)value;
    replace->move       = (uint16_t)move;
    replace->hash16     = (uint16_t)hash16;
}

PawnKingEntry* getPawnKingEntry(PawnKingTable *pktable, uint64_t pkhash) {
    PawnKingEntry *pkentry = &pktable->entries[pkhash >> 48];
    return pkentry->pkhash == pkhash ? pkentry : NULL;
}

void storePawnKingEntry(PawnKingTable *pktable, uint64_t pkhash, uint64_t passed, int eval) {
    PawnKingEntry *pkentry = &pktable->entries[pkhash >> 48];
>>>>>>> upstream/master
    pkentry->pkhash = pkhash;
    pkentry->passed = passed;
    pkentry->eval   = eval;
}
