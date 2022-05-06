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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "board.h"
#include "history.h"
#include "move.h"
#include "thread.h"
#include "types.h"


static int stat_bonus(int depth) {

    // Approximately verbatim stat bonus formula from Stockfish
    return depth > 13 ? 32 : 16 * depth * depth + 128 * MAX(depth - 1, 0);
}

static void update_history(int16_t *current, int depth, bool good) {

    // HistoryDivisor is essentially the max value of history
    const int delta = good ? stat_bonus(depth) : -stat_bonus(depth);
    *current += delta - *current * abs(delta) / HistoryDivisor;
}


static int history_captured_piece(Thread *thread, uint16_t move) {

    // Handle Enpassant; Consider promotions as Pawn Captures
    return MoveType(move) != NORMAL_MOVE ? PAWN
         : pieceType(thread->board.squares[MoveTo(move)]);
}

static int16_t* ptr_capture_history(Thread *thread, uint16_t move) {

    const int captured = history_captured_piece(thread, move);
    const int piece    = pieceType(thread->board.squares[MoveFrom(move)]);

    return &thread->chistory[piece][MoveTo(move)][captured];
}

static void ptr_quiet_history(Thread *thread, uint16_t move, int16_t *histories[3]) {

    static int16_t NULL_HISTORY; // Always zero to handle missing CM/FM history

    NodeState *const ns = &thread->states[thread->height];

    // Extract information from this move
    const int to    = MoveTo(move);
    const int from  = MoveFrom(move);
    const int piece = pieceType(thread->board.squares[from]);

    // Set Counter Move History if it exists
    histories[0] = (ns-1)->continuations == NULL
                 ? &NULL_HISTORY : &(*(ns-1)->continuations)[0][piece][to];

    // Set Followup Move History if it exists
    histories[1] = (ns-2)->continuations == NULL
                 ? &NULL_HISTORY : &(*(ns-2)->continuations)[1][piece][to];

    // Set Butterfly History, which will always exist
    histories[2] = &thread->history[thread->board.turn][from][to];
}


void update_history_heuristics(Thread *thread, uint16_t *moves, int length, int depth) {

    NodeState *const prev = &thread->states[thread->height-1];
    const int colour = thread->board.turn;

    update_killer_moves(thread, moves[length-1]);

    if (prev->move != NONE_MOVE && prev->move != NULL_MOVE)
        thread->cmtable[!colour][prev->movedPiece][MoveTo(prev->move)] = moves[length-1];

    update_quiet_histories(thread, moves, length, depth);
}

void update_killer_moves(Thread *thread, uint16_t move) {

    // Avoid saving the same Killer Move twice
    if (thread->killers[thread->height][0] != move) {
        thread->killers[thread->height][1] = thread->killers[thread->height][0];
        thread->killers[thread->height][0] = move;
    }
}

void get_refutation_moves(Thread *thread, uint16_t *killer1, uint16_t *killer2, uint16_t *counter) {

    // At each ply, we should have two potential Killer moves that have produced cutoffs
    // at the same ply in sibling nodes. Additionally, we may have a counter move, which
    // refutes the previously moved piece's destination square, somewhere in the search tree

    NodeState *const prev = &thread->states[thread->height-1];

    *counter = (prev->move == NONE_MOVE || prev->move == NULL_MOVE) ? NONE_MOVE
             :  thread->cmtable[!thread->board.turn][prev->movedPiece][MoveTo(prev->move)];

    *killer1 = thread->killers[thread->height][0];
    *killer2 = thread->killers[thread->height][1];
}


int get_capture_history(Thread *thread, uint16_t move) {

    // Inflate Queen Promotions beyond the range of reductions
    return *ptr_capture_history(thread, move)
         + 64000 * (MovePromoPiece(move) == QUEEN);
}

void get_capture_histories(Thread *thread, uint16_t *moves, int *scores, int start, int length) {

    // Grab histories for all of the capture moves. Since this is used for sorting,
    // we include an MVV-LVA factor to improve sorting. Additionally, we add 64k to
    // the history score to ensure it is >= 0 to differentiate good from bad later on

    static const int MVVAugment[] = { 0, 2400, 2400, 4800, 9600 };

    for (int i = start; i < start + length; i++)
        scores[i] = 64000 + get_capture_history(thread, moves[i])
                  + MVVAugment[history_captured_piece(thread, moves[i])];
}

void update_capture_histories(Thread *thread, uint16_t best, uint16_t *moves, int length, int depth) {

    // Update the history for each capture move that was attempted. One of them
    // might have been the move which produced a cutoff, and thus earn a bonus

    for (int i = 0; i < length; i++)
        update_history(ptr_capture_history(thread, moves[i]), depth, moves[i] == best);
}


int get_quiet_history(Thread *thread, uint16_t move, int16_t *cmhist, int16_t *fmhist) {

    int16_t butterfly = 0, *histories[3] = { cmhist, fmhist, &butterfly };
    ptr_quiet_history(thread, move, histories);
    return *cmhist + *fmhist + butterfly;
}

void get_quiet_histories(Thread *thread, uint16_t *moves, int *scores, int start, int length) {

    for (int i = start; i < start + length; i++) {
        int16_t *histories[3];
        ptr_quiet_history(thread, moves[i], histories);
        scores[i] = *histories[0] + *histories[1] + *histories[2];
    }
}

void update_quiet_histories(Thread *thread, uint16_t *moves, int length, int depth) {

    NodeState *const ns = &thread->states[thread->height];

    // We found a low-depth cutoff too easily
    if (!depth || (length == 1 && depth <= 3))
        return;

    for (int i = 0; i < length; i++) {

        int16_t *histories[3];
        ptr_quiet_history(thread, moves[i], histories);

        // Update Counter Move History if it exists
        if ((ns-1)->continuations != NULL)
             update_history(histories[0], depth, i == length - 1);

        // Update Followup Move History if it exists
        if ((ns-2)->continuations != NULL)
             update_history(histories[1], depth, i == length - 1);

        // Update Butterfly History, which always exists
        update_history(histories[2], depth, i == length - 1);
    }
}
