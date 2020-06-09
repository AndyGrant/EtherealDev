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

void updateHistories(Thread *thread, uint16_t *moves, int length, int height, int bonus) {

    // Lookup the previous two moves
    ContSegment *cmhist, *fmhist;
    uint16_t counter = thread->moveStack[height-1];
    uint16_t follow =  thread->moveStack[height-2];

    // Lookup the previous two pieces moved
    int cmPiece = thread->pieceStack[height-1];
    int fmPiece = thread->pieceStack[height-2];

    // Grab the table for counter moves if it exists
    if (counter == NONE_MOVE || counter == NULL_MOVE) cmhist = NULL;
    else cmhist = &thread->continuation[0][cmPiece][MoveTo(counter)];

    // Grab the table for followup moves if it exists
    if (follow == NONE_MOVE || follow == NULL_MOVE) fmhist = NULL;
    else fmhist = &thread->continuation[1][fmPiece][MoveTo(follow)];

    for (int i = 0; i < length - 1; i++)
        updateHistory(thread, cmhist, fmhist, moves[i], -bonus);
    updateHistory(thread, cmhist, fmhist, moves[length-1], bonus);
}

void updateHistory(Thread *thread, ContSegment *cmhist, ContSegment *fmhist, uint16_t move, int delta) {

    int to = MoveTo(move);
    int from = MoveFrom(move);
    int piece = pieceType(thread->board.squares[from]);

    int entry = thread->history[thread->board.turn][from][to];
    entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
    thread->history[thread->board.turn][from][to] = entry;

    if (cmhist != NULL) {
        entry = (*cmhist)[piece][to];
        entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
        (*cmhist)[piece][to] = entry;
    }

    if (fmhist != NULL) {
        entry = (*fmhist)[piece][to];
        entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
        (*fmhist)[piece][to] = entry;
    }
}

void updateKillerMoves(Thread *thread, int height, uint16_t move) {

    if (thread->killers[height][0] == move) return;

    thread->killers[height][1] = thread->killers[height][0];
    thread->killers[height][0] = move;
}

void updateCounterMove(Thread *thread, int height, uint16_t move) {

    uint16_t counter = thread->moveStack[height-1];
    int cmPiece = thread->pieceStack[height-1];

    if (counter != NONE_MOVE && counter != NULL_MOVE)
        thread->cmtable[thread->board.turn][cmPiece][MoveTo(counter)] = move;
}


void getHistory(Thread *thread, uint16_t move, int height, int *hist, int *cmhist, int *fmhist) {

    // Extract information from this move
    int to = MoveTo(move);
    int from = MoveFrom(move);
    int piece = pieceType(thread->board.squares[from]);

    // Extract information from last move
    uint16_t counter = thread->moveStack[height-1];
    int cmPiece = thread->pieceStack[height-1];
    int cmTo = MoveTo(counter);

    // Extract information from two moves ago
    uint16_t follow = thread->moveStack[height-2];
    int fmPiece = thread->pieceStack[height-2];
    int fmTo = MoveTo(follow);

    // Set basic Butterfly history
    *hist = thread->history[thread->board.turn][from][to];

    // Set Counter Move History if it exists
    if (counter == NONE_MOVE || counter == NULL_MOVE) *cmhist = 0;
    else *cmhist = thread->continuation[0][cmPiece][cmTo][piece][to];

    // Set Followup Move History if it exists
    if (follow == NONE_MOVE || follow == NULL_MOVE) *fmhist = 0;
    else *fmhist = thread->continuation[1][fmPiece][fmTo][piece][to];
}

void getHistoryScores(Thread *thread, uint16_t *moves, int *scores, int start, int length, int height) {

    // Extract information from last move
    uint16_t counter = thread->moveStack[height-1];
    int cmPiece = thread->pieceStack[height-1];
    int cmTo = MoveTo(counter);

    // Extract information from two moves ago
    uint16_t follow = thread->moveStack[height-2];
    int fmPiece = thread->pieceStack[height-2];
    int fmTo = MoveTo(follow);

    for (int i = start; i < start + length; i++) {

        // Extract information from this move
        int to = MoveTo(moves[i]);
        int from = MoveFrom(moves[i]);
        int piece = pieceType(thread->board.squares[from]);

        // Start with the basic Butterfly history
        scores[i] = thread->history[thread->board.turn][from][to];

        // Add Counter Move History if it exists
        if (counter != NONE_MOVE && counter != NULL_MOVE)
            scores[i] += thread->continuation[0][cmPiece][cmTo][piece][to];

        // Add Followup Move History if it exists
        if (follow != NONE_MOVE && follow != NULL_MOVE)
            scores[i] += thread->continuation[1][fmPiece][fmTo][piece][to];
    }
}

void getRefutationMoves(Thread *thread, int height, uint16_t *killer1, uint16_t *killer2, uint16_t *counter) {

    // Extract information from last move
    uint16_t previous = thread->moveStack[height-1];
    int cmPiece = thread->pieceStack[height-1];
    int cmTo = MoveTo(previous);

    // Set Killer Moves by height
    *killer1 = thread->killers[height][0];
    *killer2 = thread->killers[height][1];

    // Set Counter Move if one exists
    if (previous == NONE_MOVE || previous == NULL_MOVE) *counter = NONE_MOVE;
    else *counter = thread->cmtable[thread->board.turn][cmPiece][cmTo];
}
