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
#include "history.h"
#include "move.h"
#include "types.h"

void updateHistory(HistoryTable history, Board *board, uint16_t move, int delta) {

    int to     = MoveTo(move);
    int colour = board->turn;
    int piece  = pieceType(board->squares[MoveFrom(move)]);

    assert(0 <= to && to < SQUARE_NB);
    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= piece && piece < PIECE_NB);

    int entry  = history[colour][piece][to];

    // Cap deltas from large depths
    delta = MAX(-400, MIN(400, delta));

    // Gradual update, results abs(entry) <= 16384
    entry += 32 * delta - entry * abs(delta) / 512;

    history[colour][piece][to] = entry;
}

int getHistory(HistoryTable history, Board *board, uint16_t move) {

    int to     = MoveTo(move);
    int colour = board->turn;
    int piece  = pieceType(board->squares[MoveFrom(move)]);

    assert(0 <= to && to < SQUARE_NB);
    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= piece && piece < PIECE_NB);

    return history[colour][piece][to];
}