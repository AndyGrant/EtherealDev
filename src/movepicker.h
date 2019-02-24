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

#ifndef _MOVEPICKER_H
#define _MOVEPICKER_H

#include "types.h"

enum {
    STAGE_TABLE,
    STAGE_GENERATE_NOISY, STAGE_GOOD_NOISY,
    STAGE_KILLER_1, STAGE_KILLER_2, STAGE_COUNTER_MOVE,
    STAGE_GENERATE_QUIET, STAGE_QUIET,
    STAGE_BAD_NOISY,
    STAGE_DONE,
};

enum {
    NORMAL_PICKER,
    NOISY_PICKER,
};

struct MovePicker {
    int split, noisySize, quietSize;
    int stage, height, type, threshold;
    int values[MAX_MOVES];
    uint16_t moves[MAX_MOVES];
    uint16_t tableMove, killer1, killer2, counter;
    Thread *thread;
};

void initMovePicker(MovePicker* mp, Thread* thread, uint16_t ttMove, int height);
void initNoisyMovePicker(MovePicker* mp, Thread* thread, int threshold);
uint16_t selectNextMove(MovePicker* mp, Board* board, int skipQuiets);
int getBestMoveIndex(MovePicker *mp, int start, int end);
void evaluateNoisyMoves(MovePicker* mp);
int moveIsPsuedoLegal(Board* board, uint16_t move);

#endif
