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
#include "evaluate.h"
#include "thread.h"
#include "types.h"
#include "zobrist.h"


const uint64_t MaterialPrimes[32] = {
    17008651141875982339ull, 11695583624105689831ull, 0ull, 0ull,
    15202887380319082783ull, 13469005675588064321ull, 0ull, 0ull,
    12311744257139811149ull, 15394650811035483107ull, 0ull, 0ull,
    10979190538029446137ull, 18264461213049635989ull, 0ull, 0ull,
    11811845319353239651ull, 15484752644942473553ull, 0ull, 0ull,
};


int getCachedEvaluation(Thread *thread, Board *board, int *eval) {

    EvalEntry eve;
    uint64_t key1, key2;

    key1 =  board->turn ? board->hash ^ ZobristTurnKey : board->hash;
    eve  =  thread->evtable[key1 & EVAL_CACHE_MASK];
    key2 = (eve & ~0xFFFF) | (key1 & 0xFFFF);

    *eval = (int16_t)((uint16_t)(eve & 0xFFFF));
    *eval = Tempo + (board->turn == WHITE ? *eval : -*eval);
    return key1 == key2;
}

void storeCachedEvaluation(Thread *thread, Board *board, int eval) {
    uint64_t key1 =  board->turn ? board->hash ^ ZobristTurnKey : board->hash;
    thread->evtable[key1 & EVAL_CACHE_MASK] = (key1 & ~0xFFFF) | (uint16_t)((int16_t)eval);
}


PKEntry* getCachedPawnKingEval(Thread *thread, Board *board) {
    PKEntry *pke = &thread->pktable[board->pkhash & PK_CACHE_MASK];
    return pke->pkhash == board->pkhash ? pke : NULL;
}

void storeCachedPawnKingEval(Thread *thread, Board *board, uint64_t passed, int eval, int safetyw, int safetyb) {
    PKEntry *pke = &thread->pktable[board->pkhash & PK_CACHE_MASK];
    *pke = (PKEntry) {board->pkhash, passed, eval, safetyw, safetyb};
}


int getCachedMaterialEval(Thread *thread, Board *board, int *eval) {
    *eval = thread->mtable[board->mhash & MAT_CACHE_MASK].eval;
    return thread->mtable[board->mhash & MAT_CACHE_MASK].mhash == board->mhash;
}

void storeCachedMaterialEval(Thread *thread, Board *board, int eval) {
    MaterialEntry *mat = &thread->mtable[board->mhash & MAT_CACHE_MASK];
    *mat = (MaterialEntry) { board->mhash, eval };
}

