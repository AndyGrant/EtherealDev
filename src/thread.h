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

#pragma once

#include <setjmp.h>
#include <stdint.h>

#include "board.h"
#include "search.h"
#include "transposition.h"
#include "types.h"

enum {
    STACK_OFFSET = 4,
    STACK_SIZE = MAX_PLY + STACK_OFFSET
};

struct Thread {

    Board board;
    Limits *limits;
    SearchInfo *info;

    int value;     // TODO: Delete
    PVariation pv; // TODO: Delete

    int values[256];
    PVariation pvs[256];

    int depth, seldepth;
    uint64_t nodes, tbhits;

    // As we go down the search tree we track an evaluation at each
    // node, as well as the moves and the pieces moved in order to
    // get to that position. STACK_SIZE is greater than MAX_PLY, in
    // order to index "negative" values to look before the root node

    int *evalStack, _evalStack[STACK_SIZE];
    uint16_t *moveStack, _moveStack[STACK_SIZE];
    int *pieceStack, _pieceStack[STACK_SIZE];
    Undo undoStack[STACK_SIZE];

    // Evaluation Tables, Move Tables, and History Counters.
    // Each thread manages their own set of these tables.

    PKTable pktable;
    KillerTable killers;
    CounterMoveTable cmtable;
    HistoryTable history;
    ContinuationTable continuation;

    // We retain a reference to the other threads as well as the number
    // of threads in total. We know our place in the thread pool.

    int index, nthreads;
    Thread *threads;

    // In order to quickly exit a search and return to the main loop
    // we have a jmp_buf. This is used when search time has expired,
    // or when the main thread has signaled the helper threads to stop.

    jmp_buf jbuffer;
};


Thread* createThreadPool(int nthreads);
void resetThreadPool(Thread *threads);
void newSearchThreadPool(Thread *threads, Board *board, Limits *limits, SearchInfo *info);
uint64_t nodesSearchedThreadPool(Thread *threads);
uint64_t tbhitsThreadPool(Thread *threads);
