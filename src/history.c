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
#include "thread.h"
#include "types.h"

int getHistory(Thread *thread, uint16_t move) {

    int colour = thread->board.turn;
    int from   = MoveFrom(move);
    int to     = MoveTo(move);

    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= from && from < SQUARE_NB);
    assert(0 <= to && to < SQUARE_NB);

    return thread->history[colour][from][to];
}

void updateHistory(Thread *thread, uint16_t move, int delta) {

    int entry;
    int colour = thread->board.turn;
    int from   = MoveFrom(move);
    int to     = MoveTo(move);

    // Bound update by [HistoryMin, HistoryMax]
    delta = MAX(HistoryMin, MIN(HistoryMax, delta));

    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= from && from < SQUARE_NB);
    assert(0 <= to && to < SQUARE_NB);

    entry = thread->history[colour][from][to];
    entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
    thread->history[colour][from][to] = entry;
}

int getContinuationHistory(Thread *thread, int height, uint16_t move, int plies) {

    int to1, to2, piece1, piece2;
    uint16_t continuing = thread->moveStack[height-plies];

    // No history when following non moves
    if (continuing == NONE_MOVE || continuing == NULL_MOVE)
        return 0;

    // Find piece types and destination squares
    to1 = MoveTo(continuing), to2 = MoveTo(move);
    piece1 = thread->pieceStack[height-plies];
    piece2 = pieceType(thread->board.squares[MoveFrom(move)]);

    assert(1 <= plies && plies <= CONT_NB);
    assert(0 <= piece1 && piece1 < PIECE_NB);
    assert(0 <= to1 && to1 < SQUARE_NB);
    assert(0 <= piece2 && piece2 < PIECE_NB);
    assert(0 <= to2 && to2 < SQUARE_NB);

    return thread->continuation[plies-1][piece1][to1][piece2][to2];
}

void updateContinuationHistory(Thread *thread, int height, uint16_t move, int plies, int delta) {

    int to1, to2, piece1, piece2, entry;
    uint16_t continuing = thread->moveStack[height-plies];

    // No history when following non moves
    if (continuing == NONE_MOVE || continuing == NULL_MOVE)
        return;

    // Find piece types and destination squares
    to1 = MoveTo(continuing), to2 = MoveTo(move);
    piece1 = thread->pieceStack[height-plies];
    piece2 = pieceType(thread->board.squares[MoveFrom(move)]);

    // Bound update by [HistoryMin, HistoryMax]
    delta = MAX(HistoryMin, MIN(HistoryMax, delta));

    assert(1 <= plies && plies <= CONT_NB);
    assert(0 <= piece1 && piece1 < PIECE_NB);
    assert(0 <= to1 && to1 < SQUARE_NB);
    assert(0 <= piece2 && piece2 < PIECE_NB);
    assert(0 <= to2 && to2 < SQUARE_NB);

    entry = thread->continuation[plies-1][piece1][to1][piece2][to2];
    entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
    thread->continuation[plies-1][piece1][to1][piece2][to2] = entry;
}

uint16_t getCounterMove(Thread *thread, int height) {

    int colour, to, piece;
    const uint16_t previous = thread->moveStack[height-1];

    // Check for root position or null moves
    if (previous == NULL_MOVE || previous == NONE_MOVE)
        return NONE_MOVE;

    colour = !thread->board.turn;
    to     = MoveTo(previous);
    piece  = pieceType(thread->board.squares[to]);

    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= piece && piece < PIECE_NB);
    assert(0 <= to && to < SQUARE_NB);

    return thread->cmtable[colour][piece][to];
}

void updateCounterMove(Thread *thread, int height, uint16_t move) {

    int colour, to, piece;
    const uint16_t previous = thread->moveStack[height-1];

    // Check for root position or null moves
    if (previous == NULL_MOVE || previous == NONE_MOVE)
        return;

    colour = !thread->board.turn;
    to     = MoveTo(previous);
    piece  = pieceType(thread->board.squares[to]);

    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= piece && piece < PIECE_NB);
    assert(0 <= to && to < SQUARE_NB);

    thread->cmtable[colour][piece][to] = move;
}
