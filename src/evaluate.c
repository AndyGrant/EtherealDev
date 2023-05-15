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
#include "bitboards.h"
#include "evaluate.h"
#include "move.h"
#include "nnue/nnue.h"
#include "thread.h"
#include "types.h"

int evaluateBoard(Thread *thread, Board *board) {

    const int Tempo = 20;

    int phase, eval;

    // We can recognize positions we just evaluated
    if (thread->states[thread->height-1].move == NULL_MOVE)
        return -thread->states[thread->height-1].eval + 2 * Tempo;

    // Use the NNUE, and ensure we have the White POV
    eval = nnue_evaluate(thread, board);
    eval = board->turn == WHITE ? eval : -eval;

    // Calculate the game phase based on remaining material (Fruit Method)
    phase = 4 * popcount(board->pieces[QUEEN ])
          + 2 * popcount(board->pieces[ROOK  ])
          + 1 * popcount(board->pieces[KNIGHT]|board->pieces[BISHOP]);

    // Compute and store an interpolated evaluation from white's POV
    eval = (ScoreMG(eval) * phase + ScoreEG(eval) * (24 - phase)) / 24;

    // Factor in the Tempo after interpolation and scaling, so that
    // if a null move is made, then we know eval = last_eval + 2 * Tempo
    return Tempo + (board->turn == WHITE ? eval : -eval);
}