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

#include <assert.h>
#include <stdlib.h>

#include "attacks.h"
#include "board.h"
#include "bitboards.h"
#include "castle.h"
#include "evaluate.h"
#include "history.h"
#include "move.h"
#include "movegen.h"
#include "movepicker.h"
#include "types.h"
#include "thread.h"

static void evaluateNoisyMoves(MovePicker *mp) {

    // Use modified MVV-LVA to evaluate moves
    for (int i = 0; i < mp->noisySize; i++){

        int fromType = pieceType(mp->thread->board.squares[MoveFrom(mp->moves[i])]);
        int toType   = pieceType(mp->thread->board.squares[  MoveTo(mp->moves[i])]);

        // Use the standard MVV-LVA
        mp->values[i] = PieceValues[toType][EG] - fromType;

        // Factor in promotion material gain only for queen promotions
        if (MovePromoType(mp->moves[i]) == PROMOTE_TO_QUEEN)
            mp->values[i] += PieceValues[QUEEN][EG];

        // Due to our encoding, standard MVV-LVA does not find enpass
        else if (MoveType(mp->moves[i]) == ENPASS_MOVE)
            mp->values[i] = PieceValues[PAWN][EG] - PAWN;

        // We will flag moves which are skipped during STAGE_GOOD_NOISY with a
        // value of -1. Thus, all values are assumed to start at or above zero
        assert(mp->values[i] >= 0);
    }
}

static void evaluateQuietMoves(MovePicker *mp) {

    // Evaluate based on Butterfly history (Colour, From, To), and
    // Continuation History (Counter Move, Followup Move histories)

    for (int i = mp->split; i < mp->split + mp->quietSize; i++)
        mp->values[i] = getHistoryScore(mp->thread, mp->moves[i])
                      + getCMHistoryScore(mp->thread, mp->height, mp->moves[i])
                      + getFUHistoryScore(mp->thread, mp->height, mp->moves[i]);
}

static int getBestMoveIndex(MovePicker *mp, int start, int end) {

    int best = start;

    for (int i = start + 1; i < end; i++)
        if (mp->values[i] > mp->values[best])
            best = i;

    return best;
}

static uint16_t popMove(MovePicker *mp, int *counter, int index, int end) {

    // Return the move located at the given index, and shift the move
    // list by placing the last item into the newly created hole

    const uint16_t best = mp->moves[index];

    *counter = *counter - 1;
    mp->moves[index] = mp->moves[end-1];
    mp->values[index] = mp->values[end-1];
    return best;
}

static int moveIsPsuedoLegal(Board *board, uint16_t move) {

    const int colour = board->turn;
    const int to     = MoveTo(move);
    const int from   = MoveFrom(move);
    const int type   = MoveType(move);
    const int ptype  = MovePromoType(move);
    const int ftype  = pieceType(board->squares[from]);

    const uint64_t friendly  = board->colours[ colour];
    const uint64_t enemy     = board->colours[!colour];
    const uint64_t occupied  = friendly | enemy;
    const uint64_t thirdRank = colour == WHITE ? RANK_3 : RANK_6;

    // Check for moves which are clearly invalid, made using a piece which
    // does not belong to the current player, or breaks the standard defined
    // in Ethereal for non promotion moves having no promotion flags set
    if (    move == NONE_MOVE ||  move == NULL_MOVE
        ||  pieceColour(board->squares[from]) != colour
        || (ptype != PROMOTE_TO_KNIGHT && type != PROMOTION_MOVE))
        return 0;

    if (ftype == PAWN){

        // Pawns cannot be apart of a castle move
        if (type == CASTLE_MOVE) return 0;

        // Find our potential attacks and advances
        uint64_t attacks = pawnAttacks(colour, from);
        uint64_t forward = pawnAdvance(1ull << from, occupied, colour);

        // Enpass moves are legal when the target square and the enpass
        // square are matching, and the target square is a potential attack
        if (type == ENPASS_MOVE)
            return to == board->epSquare && testBit(attacks, to);

        // Promotions moves are legal when the target square is a promotion
        // square, and exists as a valid potential attack for the given pawn
        if (type == PROMOTION_MOVE)
            return testBit(PROMOTION_RANKS & (forward | (attacks & enemy)), to);

        // Normal moves are legal when the target square is not a promotion
        // square, and exists as a valid potential attack for the given pawn
        forward |= pawnAdvance(forward & thirdRank, occupied, colour);
        return testBit(~PROMOTION_RANKS & (forward | (attacks & enemy)), to);
    }

    // Minor and Major moves are legal when the move type is normal.
    // The special moves allowed at this point will belong to the King

    if (type != NORMAL_MOVE && ftype != KING) return 0;
    if (ftype == KNIGHT) return testBit(knightAttacks(from) & ~friendly, to);
    if (ftype == BISHOP) return testBit(bishopAttacks(from, occupied) & ~friendly, to);
    if (ftype == ROOK  ) return testBit(rookAttacks(from, occupied) & ~friendly, to);
    if (ftype == QUEEN ) return testBit(queenAttacks(from, occupied) & ~friendly, to);

    // Remainder of possible moves must be for Kings, via process of elimination,
    // since Ethereal only allows for pieces and the EMPTY value for board->squares,
    // the from type must either be a King or a completely undefined value
    assert(ftype == KING); assert(type == NORMAL_MOVE || type == CASTLE_MOVE);

    // Normal moves are legal when the target exists as a valid potential attack
    if (type == NORMAL_MOVE) return testBit(kingAttacks(from) & ~friendly, to);

    // Kings cannot be apart of an enpass or promotion move
    if (type == ENPASS_MOVE || type == PROMOTION_MOVE) return 0;

    // Only remaining moves that are candidates are castle moves. We can simply
    // check for equality between the move and each of the four possible castling
    // moves. If a match occurs, we can just return the psuedo legality for the castle

    if (move == OO_WHITE_MOVE)
        return   colour == WHITE
            &&  !board->kingAttackers
            && !(occupied & WHITE_CASTLE_KING_SIDE_MAP)
            &&  (board->castleRights & WHITE_KING_RIGHTS)
            &&  !squareIsAttacked(board, WHITE, MoveTo(OO_WHITE_MOVE)-1);

    if (move == OOO_WHITE_MOVE)
        return   colour == WHITE
            &&  !board->kingAttackers
            && !(occupied & WHITE_CASTLE_QUEEN_SIDE_MAP)
            &&  (board->castleRights & WHITE_QUEEN_RIGHTS)
            &&  !squareIsAttacked(board, WHITE, MoveTo(OOO_WHITE_MOVE)+1);

    if (move == OO_BLACK_MOVE)
        return   colour == BLACK
            &&  !board->kingAttackers
            && !(occupied & BLACK_CASTLE_KING_SIDE_MAP)
            &&  (board->castleRights & BLACK_KING_RIGHTS)
            &&  !squareIsAttacked(board, BLACK, MoveTo(OO_BLACK_MOVE)-1);

    if (move == OOO_BLACK_MOVE)
        return   colour == BLACK
            &&  !board->kingAttackers
            && !(occupied & BLACK_CASTLE_QUEEN_SIDE_MAP)
            &&  (board->castleRights & BLACK_QUEEN_RIGHTS)
            &&  !squareIsAttacked(board, BLACK, MoveTo(OOO_BLACK_MOVE)+1);

    return 0; // No matching moves found
}


void initMovePicker(MovePicker *mp, Thread *thread, uint16_t ttMove, int height) {

    // Start with the table move
    mp->stage = STAGE_TABLE;

    // Save possible special moves
    mp->tableMove = ttMove;
    mp->killer1   = thread->killers[height][0];
    mp->killer2   = thread->killers[height][1];
    mp->counter   = getCounterMove(thread, height);

    // Threshold for good noisy
    mp->threshold = 0;

    // Reference to the board
    mp->thread = thread;

    // Reference for getting stats
    mp->height = height;

    // Normal picker returns bad noisy moves
    mp->type = NORMAL_PICKER;
}

void initNoisyMovePicker(MovePicker *mp, Thread *thread, int threshold) {

    // Start with just the noisy moves
    mp->stage = STAGE_GENERATE_NOISY;

    // Skip all special moves
    mp->tableMove = NONE_MOVE;
    mp->killer1   = NONE_MOVE;
    mp->killer2   = NONE_MOVE;
    mp->counter   = NONE_MOVE;

    // Threshold for good noisy
    mp->threshold = threshold;

    // Reference to the board
    mp->thread = thread;

    // No stats used, set to 0 to be safe
    mp->height = 0;

    // Noisy picker skips bad noisy moves
    mp->type = NOISY_PICKER;
}

uint16_t selectNextMove(MovePicker *mp, Board *board, int skipQuiets) {

    int best;
    uint16_t bestMove;

    switch (mp->stage){

    case STAGE_TABLE:

        // Play table move if it is psuedo legal
        mp->stage = STAGE_GENERATE_NOISY;
        if (moveIsPsuedoLegal(board, mp->tableMove))
            return mp->tableMove;

        /* fallthrough */

    case STAGE_GENERATE_NOISY:

        // Generate and evaluate all noisy moves
        genAllNoisyMoves(board, mp->moves, &mp->noisySize);
        evaluateNoisyMoves(mp);
        mp->stage = STAGE_GOOD_NOISY;

        /* fallthrough */

    case STAGE_GOOD_NOISY:

        if (mp->noisySize){

            while (1) {

                // Select best move based on MVV-LVA
                best = getBestMoveIndex(mp, 0, mp->noisySize);

                // Best move has been flagged as a bad capture
                if (mp->values[best] < 0) {
                    mp->stage = STAGE_KILLER_1;
                    return selectNextMove(mp, board, skipQuiets);
                }

                // Selected move was able to pass an SEE(mp->threshold)
                if (staticExchangeEvaluation(board, mp->moves[best], mp->threshold)) {
                    bestMove = popMove(mp, &mp->noisySize, best, mp->noisySize);
                    break;
                }

                // Flag this move as being a bad capture
                mp->values[best] = -1;
            }

            // Don't play the table move twice
            if (bestMove == mp->tableMove)
                return selectNextMove(mp, board, skipQuiets);

            // Don't play the special moves twice
            if (bestMove == mp->killer1) mp->killer1 = NONE_MOVE;
            if (bestMove == mp->killer2) mp->killer2 = NONE_MOVE;
            if (bestMove == mp->counter) mp->counter = NONE_MOVE;

            return bestMove;
        }

        // Jump to bad noisy moves when skipping quiets
        if (skipQuiets){
            mp->stage = STAGE_BAD_NOISY;
            return selectNextMove(mp, board, skipQuiets);
        }

        mp->stage = STAGE_KILLER_1;

        /* fallthrough */

    case STAGE_KILLER_1:

        // Play killer move if not yet played and psuedo legal
        mp->stage = STAGE_KILLER_2;
        if (   !skipQuiets
            &&  mp->killer1 != mp->tableMove
            &&  moveIsPsuedoLegal(board, mp->killer1))
            return mp->killer1;

        /* fallthrough */

    case STAGE_KILLER_2:

        // Play killer move if not yet played and psuedo legal
        mp->stage = STAGE_COUNTER_MOVE;
        if (   !skipQuiets
            &&  mp->killer2 != mp->tableMove
            &&  moveIsPsuedoLegal(board, mp->killer2))
            return mp->killer2;

        /* fallthrough */

    case STAGE_COUNTER_MOVE:

        // Play counter move if not yet played and psuedo legal
        mp->stage = STAGE_GENERATE_QUIET;
        if (   !skipQuiets
            &&  mp->counter != mp->tableMove
            &&  mp->counter != mp->killer1
            &&  mp->counter != mp->killer2
            &&  moveIsPsuedoLegal(board, mp->counter))
            return mp->counter;

        /* fallthrough */

    case STAGE_GENERATE_QUIET:

        // Generate and evaluate all quiet moves. mp->split is used
        // to cut the internal move list into one partition for the
        // noisy moves and another partition for the quiet ones.

        if (!skipQuiets) {
            mp->split = mp->noisySize;
            genAllQuietMoves(board, mp->moves + mp->split, &mp->quietSize);
            evaluateQuietMoves(mp);
        }

        mp->stage = STAGE_QUIET;

        /* fallthrough */

    case STAGE_QUIET:

        if (mp->quietSize && !skipQuiets){

            // Select next best quiet by history scores
            best = getBestMoveIndex(mp, mp->split, mp->quietSize + mp->split);
            bestMove = popMove(mp, &mp->quietSize, best, mp->quietSize + mp->split);

            // Don't play a move more than once
            if (   bestMove == mp->tableMove
                || bestMove == mp->killer1
                || bestMove == mp->killer2
                || bestMove == mp->counter)
                return selectNextMove(mp, board, skipQuiets);

            return bestMove;
        }

        // Out of quiet moves, only bad quiets remain
        mp->stage = STAGE_BAD_NOISY;

        /* fallthrough */

    case STAGE_BAD_NOISY:

        // We skip all bad noisy moves when using a NOISY_PICKER
        if (mp->noisySize && mp->type != NOISY_PICKER){

            // Return moves one at a time without sorting
            bestMove = popMove(mp, &mp->noisySize, 0, mp->noisySize);

            // Don't play a move more than once
            if (   bestMove == mp->tableMove
                || bestMove == mp->killer1
                || bestMove == mp->killer2
                || bestMove == mp->counter)
                return selectNextMove(mp, board, skipQuiets);

            return bestMove;
        }

        // Out of all captures and quiet moves, move picker complete
        mp->stage = STAGE_DONE;

        /* fallthrough */

    case STAGE_DONE:
        return NONE_MOVE;

    default:
        assert(0);
        return NONE_MOVE;
    }
}
