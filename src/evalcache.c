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

int getCachedEvaluation(Thread *thread, Board *board, int *eval, int useNNUE) {

    EvalEntry entry;
    uint64_t key, adjusted;

    // Ignore STM when not using NNUE
    adjusted = useNNUE ? board->hash
             : board->turn == WHITE ? board->hash
             : board->hash ^ ZobristTurnKey;

    // Fetch the Entry and extract the stored Key
    entry = thread->evtable[adjusted & EVAL_CACHE_MASK];
    key   = (entry & ~EVAL_CACHE_MASK) | (adjusted & EVAL_CACHE_MASK);

    // Extract the eval and put it in the correct POV
    *eval = (int16_t)((uint16_t)(entry & EVAL_CACHE_MASK));
    *eval = board->turn == WHITE ? *eval : -*eval;

    // Lookup was only valid if the keys matched
    return adjusted == key;
}

void storeCachedEvaluation(Thread *thread, Board *board, int eval, int useNNUE) {

    uint64_t adjusted;

    // Ignore STM when not using NNUE
    adjusted = useNNUE ? board->hash
             : board->turn == WHITE ? board->hash
             : board->hash ^ ZobristTurnKey;

    // Store the Entry and encode (Key, Eval)
    thread->evtable[adjusted & EVAL_CACHE_MASK]
        = (adjusted & ~EVAL_CACHE_MASK) | (uint16_t)((int16_t)eval);
}


PKEntry* getCachedPawnKingEval(Thread *thread, Board *board) {
    PKEntry *pke = &thread->pktable[board->pkhash & PK_CACHE_MASK];
    return pke->pkhash == board->pkhash ? pke : NULL;
}

void storeCachedPawnKingEval(Thread *thread, Board *board, uint64_t passed, int eval, int safetyw, int safetyb) {
    PKEntry *pke = &thread->pktable[board->pkhash & PK_CACHE_MASK];
    *pke = (PKEntry) {board->pkhash, passed, eval, safetyw, safetyb};
}

