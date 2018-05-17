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

#ifndef _SEARCH_H
#define _SEARCH_H

#include <stdint.h>

#include "types.h"

struct SearchInfo {
<<<<<<< HEAD
    
=======

>>>>>>> upstream/master
    int depth;
    int values[MAX_PLY];
    uint16_t bestMoves[MAX_PLY];
    double timeUsage[MAX_PLY];
<<<<<<< HEAD
    
=======

>>>>>>> upstream/master
    double startTime;
    double idealUsage;
    double maxAlloc;
    double maxUsage;
<<<<<<< HEAD
    
=======

>>>>>>> upstream/master
    int bestMoveChanges;
};

struct PVariation {
    uint16_t line[MAX_PLY];
    int length;
};


uint16_t getBestMove(Thread* threads, Board* board, Limits* limits);

void* iterativeDeepening(void* vthread);

int aspirationWindow(Thread* thread, int depth);

int search(Thread* thread, PVariation* pv, int alpha, int beta, int depth, int height);

int qsearch(Thread* thread, PVariation* pv, int alpha, int beta, int height);

int staticExchangeEvaluation(Board* board, uint16_t move, int threshold);

int moveIsTactical(Board* board, uint16_t move);

int hasNonPawnMaterial(Board* board, int turn);

int valueFromTT(int value, int height);

int valueToTT(int value, int height);

int thisTacticalMoveValue(Board* board, uint16_t move);
<<<<<<< HEAD
    
=======

>>>>>>> upstream/master
int bestTacticalMoveValue(Board* board, EvalInfo* ei);

int captureIsWeak(Board* board, EvalInfo* ei, uint16_t move, int depth);

<<<<<<< HEAD
int moveIsSingular(Thread* thread, Board* board, TransEntry* ttEntry, Undo* undo, int depth, int height);
=======
int moveIsSingular(Thread* thread, uint16_t ttMove, int ttValue, Undo* undo, int depth, int height);
>>>>>>> upstream/master


static const int RazorDepth = 4;

static const int RazorMargins[] = {0, 300, 350, 410, 500};

static const int BetaPruningDepth = 8;

static const int BetaMargin = 85;

static const int NullMovePruningDepth = 2;

static const int ProbCutDepth = 5;

static const int ProbCutMargin = 100;

static const int IIDDepth = 3;

static const int FutilityMargin = 100;

static const int FutilityPruningDepth = 8;

static const int LateMovePruningDepth = 8;

static const int LateMovePruningCounts[] = {0, 4, 6, 10, 15, 23, 31, 40, 52};

static const int WeakCaptureTwoAttackersDepth = 5;

static const int WeakCaptureOneAttackersDepth = 3;

static const int SEEPruningDepth = 8;

static const int SEEMargin[2] = { -20, -30};

static const int QFutilityMargin = 100;

static const int QSEEMargin = -100;

#endif
