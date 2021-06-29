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
#include <stdio.h>

#include "board.h"
#include "evaluate.h"
#include "move.h"
#include "nnue/nnue.h"
#include "thread.h"
#include "types.h"

int evaluateBoard(Thread *thread, Board *board) {

    static const int Tempo = 20;

    int eval, hashed;

    // We can recognize positions we just evaluated
    if (thread->moveStack[thread->height-1] == NULL_MOVE)
        return -thread->evalStack[thread->height-1] + 2 * Tempo;

    // Check for this evaluation being cached already
    if (getCachedEvaluation(thread, board, &hashed))
        return hashed + Tempo;

    // On some-what balanced positions, use just NNUE
    {
        eval = nnue_evaluate(thread, board);
        hashed = board->turn == WHITE ? eval : -eval;
        storeCachedEvaluation(thread, board, hashed);
        return eval + Tempo;
    }
}
