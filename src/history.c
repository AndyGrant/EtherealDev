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


void updateHistoryHeuristics(Thread *thread, uint16_t *moves, int length, int height, int bonus) {

    // Update the following history heuristics
    // 1. Butterfly History
    // 2. Counter Move History
    // 3. Followup Move History
    // 4. Killer Move Refutations
    // 5. Counter Move Refutations

    int entry, colour = thread->board.turn;
    uint16_t bestMove = moves[length-1];

    // Extract information from last move
    uint16_t countering = thread->moveStack[height-1];
    int counteringPiece = thread->pieceStack[height-1];
    int counteringTo = MoveTo(countering);

    // Extract information from two moves ago
    uint16_t following = thread->moveStack[height-2];
    int followingPiece = thread->pieceStack[height-2];
    int followingTo = MoveTo(following);

    // Simple assert check on all array bounds
    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= counteringPiece && counteringPiece < PIECE_NB);
    assert(0 <= counteringTo && counteringTo < SQUARE_NB);
    assert(0 <= followingPiece && followingPiece < PIECE_NB);
    assert(0 <= followingTo && followingTo < SQUARE_NB);

    // Cap update size to avoid saturation
    bonus = MIN(bonus, HistoryMax);

    for (int i = 0; i < length; i++) {

        // Apply a malus until the final move
        int delta = (moves[i] == bestMove) ? bonus : -bonus;

        // Extract information from this move
        int from = MoveFrom(moves[i]);
        int to = MoveTo(moves[i]);
        int piece = pieceType(thread->board.squares[from]);

        // Simple assert check on all array bounds
        assert(0 <= from && from < SQUARE_NB);
        assert(0 <= to && to < SQUARE_NB);
        assert(0 <= piece && piece < PIECE_NB);

        // Update Butterfly History
        entry = thread->history[colour][from][to];
        entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
        thread->history[colour][from][to] = entry;

        // Update Counter Move History
        if (countering != NONE_MOVE && countering != NULL_MOVE) {
            entry = thread->continuation[0][counteringPiece][counteringTo][piece][to];
            entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
            thread->continuation[0][counteringPiece][counteringTo][piece][to] = entry;
        }

        // Update Followup Move History
        if (following != NONE_MOVE && following != NULL_MOVE) {
            entry = thread->continuation[1][followingPiece][followingTo][piece][to];
            entry += HistoryMultiplier * delta - entry * abs(delta) / HistoryDivisor;
            thread->continuation[1][followingPiece][followingTo][piece][to] = entry;
        }
    }


    // Update Killer Moves (Avoid duplicates)
    if (thread->killers[height][0] != bestMove) {
        thread->killers[height][1] = thread->killers[height][0];
        thread->killers[height][0] = bestMove;
    }

    // Update Counter Moves (BestMove refutes the previous move)
    if (countering != NONE_MOVE && countering != NULL_MOVE)
        thread->cmtable[!colour][counteringPiece][counteringTo] = bestMove;
}

int getHistory(Thread *thread, uint16_t move) {

    int colour = thread->board.turn;
    int from   = MoveFrom(move);
    int to     = MoveTo(move);

    assert(0 <= colour && colour < COLOUR_NB);
    assert(0 <= from && from < SQUARE_NB);
    assert(0 <= to && to < SQUARE_NB);

    return thread->history[colour][from][to];
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