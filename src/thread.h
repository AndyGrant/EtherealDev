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
#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "evalcache.h"
#include "network.h"
#include "search.h"
#include "transposition.h"
#include "types.h"

enum {
    STACK_OFFSET = 4,
    STACK_SIZE = MAX_PLY + STACK_OFFSET
};

typedef struct DirtyPiece {
  int dirtyNum;
  int pc[3], from[3], to[3];
} DirtyPiece;

typedef struct Accumulator {
  ALIGN64 int16_t accumulation[COLOUR_NB][256];
  bool computedAccumulation;
} Accumulator;

typedef struct NNUEStack {
    Accumulator accumulator;
    DirtyPiece dirtyPiece;
} NNUEStack;

struct Thread {

    Board board;
    PVariation pv;
    Limits *limits;
    SearchInfo *info;

    int multiPV, values[MAX_MOVES];
    uint16_t bestMoves[MAX_MOVES];
    uint16_t ponderMoves[MAX_MOVES];

    int contempt;
    int depth, seldepth, height;
    uint64_t nodes, tbhits;

    int      *evalStack,  _evalStack  [STACK_SIZE];
    uint16_t *moveStack,  _moveStack  [STACK_SIZE];
    int      *pieceStack, _pieceStack [STACK_SIZE];
    NNUEStack *nnueStack,  *_nnueStack;

    Undo undoStack[STACK_SIZE];

    EvalTable evtable;
    PKTable pktable;

    KillerTable killers;
    CounterMoveTable cmtable;

    HistoryTable history;
    CaptureHistoryTable chistory;
    ContinuationTable continuation;

    int index, nthreads;
    Thread *threads;
    jmp_buf jbuffer;
};


Thread* createThreadPool(int nthreads);
void resetThreadPool(Thread *threads);
void deleteThreadPool(Thread *threads);

void newSearchThreadPool(Thread *threads, Board *board, Limits *limits, SearchInfo *info);
uint64_t nodesSearchedThreadPool(Thread *threads);
uint64_t tbhitsThreadPool(Thread *threads);
