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

#ifndef _SEARCH_H
#define _SEARCH_H

#include <stdint.h>

#include "types.h"

typedef struct SearchInfo {
    
    int depth;
    int values[MAX_DEPTH];
    int bestmoves[MAX_DEPTH];
    int timeUsage[MAX_DEPTH];
    
    double starttime;
    double idealusage;
    double maxalloc;
    double maxusage;
    
    double pvStability;
    double scoreStability;
    
} SearchInfo;

typedef struct PVariation {
    uint16_t line[MAX_HEIGHT];
    int length;
} PVariation;


uint16_t getBestMove(Thread* threads, Board* board, Limits* limits, double time, double mtg, double inc);

void* iterativeDeepening(void* vthread);

int aspirationWindow(Thread* thread, int depth);

int search(Thread* thread, PVariation* pv, int alpha, int beta, int depth, int height);

int qsearch(Thread* thread, PVariation* pv, int alpha, int beta, int height);

int moveIsTactical(Board* board, uint16_t move);

int hasNonPawnMaterial(Board* board, int turn);

int valueFromTT(int value, int height);

int valueToTT(int value, int height);

int thisTacticalMoveValue(Board* board, uint16_t move);
    
int bestTacticalMoveValue(Board* board);


static const int RazorDepth = 4;

static const int RazorMargins[] = {0, 300, 350, 410, 500};

static const int BetaPruningDepth = 8;

static const int FutilityMargin = 85;

static const int ProbCutDepth = 5;

static const int ProbCutMargin = 156;

static const int InternalIterativeDeepeningDepth = 3;

static const int NullMovePruningDepth = 2;

static const int FutilityPruningDepth = 8;

static const int LateMovePruningDepth = 15;

static const int LateMovePruningCounts[] = {
//[ 1] 23744017 23578438 99.3026
//[ 2] 13397683 13359378 99.7141
//[ 3]  8856096  8844842 99.8729
//[ 4]  4816964  4813244 99.9228
//[ 5]  2616026  2614820 99.9539
//[ 6]  1284293  1283953 99.9735
//[ 7]   524207   524117 99.9828
//[ 8]   194489   194462 99.9861
//[ 9]    70196    70189 99.9900
//[10]    18559    18558 99.9946
//[11]     3438     3437 99.9709
//[12]      164      164 100.0000
//[13]        9        9 100.0000
//   0,   5,   7,   9,
//  11,  14,  17,  20,
//  24,  28,  32,  37,
//  43,  48,  54,  60,
    
// [ 1] 23744017 23578438 99.3026
// [ 2] 13397683 13359378 99.7141
// [ 3]  8856096  8844842 99.8729
// [ 4]  4559208  4555955 99.9286
// [ 5]  2453029  2451993 99.9578
// [ 6]  1097212  1096951 99.9762
// [ 7]   389985   389936 99.9874
// [ 8]   111401   111387 99.9874
// [ 9]    27485    27483 99.9927
// [10]     2935     2935 100.0000
// [11]      290      290 100.0000
     0,   5,   7,   9, 
    12,  15,  19,  23,
    28,  33,  39,  45,
    52,  59,  67,  75,
    84,  93, 103, 113
};

static const int QFutilityMargin = 100;

#endif
