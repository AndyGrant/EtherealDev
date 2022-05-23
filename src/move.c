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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "masks.h"
#include "move.h"
#include "movegen.h"
#include "search.h"
#include "thread.h"
#include "types.h"
#include "uci.h"
#include "zobrist.h"

#include "nnue/accumulator.h"
#include "nnue/types.h"

static void updateCastleZobrist(Board *board, uint64_t oldRooks, uint64_t newRooks) {
    uint64_t diff = oldRooks ^ newRooks;
    while (diff)
        board->hash ^= ZobristCastleKeys[poplsb(&diff)];
}

int castleKingTo(int king, int rook) {
    return square(rankOf(king), (rook > king) ? 6 : 2);
}

int castleRookTo(int king, int rook) {
    return square(rankOf(king), (rook > king) ? 5 : 3);
}


void apply(Thread *thread, Board *board, uint16_t move) {

    NodeState *const ns = &thread->states[thread->height];

    if (move == NULL_MOVE) {
        ns->movedPiece    = EMPTY;
        ns->tactical      = false;
        ns->continuations = NULL;
        ns->move          = NULL_MOVE;
        applyNullMove(board, &thread->undoStack[thread->height]);
    }

    else {
        ns->movedPiece    = pieceType(board->squares[MoveFrom(move)]);
        ns->tactical      = moveIsTactical(board, move);
        ns->continuations = &thread->continuation[ns->tactical][ns->movedPiece][MoveTo(move)];
        ns->move          = move;
        applyMove(board, move, &thread->undoStack[thread->height]);
    }

    thread->height++;
}

void applyMove(Board *board, uint16_t move, Undo *undo) {

    static void (*table[4])(Board*, uint16_t, Undo*) = {
        applyNormalMove, applyCastleMove,
        applyEnpassMove, applyPromotionMove
    };

    // Save information which is hard to recompute
    undo->hash            = board->hash;
    undo->pkhash          = board->pkhash;
    undo->checkers        = board->checkers;
    undo->threats         = board->threats;
    undo->blockers        = board->blockers;
    undo->castleRooks     = board->castleRooks;
    undo->epSquare        = board->epSquare;
    undo->halfMoveCounter = board->halfMoveCounter;
    undo->psqtmat         = board->psqtmat;

    // Store hash history for repetition checking
    board->history[board->numMoves++] = board->hash;
    board->fullMoveCounter++;

    // Update the hash for before changing the enpass square
    if (board->epSquare != -1)
        board->hash ^= ZobristEnpassKeys[fileOf(board->epSquare)];

    // Run the correct move application function
    table[MoveType(move) >> 12](board, move, undo);

    // No function updated epsquare so we reset
    if (board->epSquare == undo->epSquare)
        board->epSquare = -1;

    // No function updates this so we do it here
    board->turn = !board->turn;

    // Compute Checkers and threats
    update_board_state(board);
}

void applyNormalMove(Board *board, uint16_t move, Undo *undo) {

    const int from = MoveFrom(move);
    const int to = MoveTo(move);

    const int fromPiece = board->squares[from];
    const int toPiece = board->squares[to];

    const int fromType = pieceType(fromPiece);
    const int toType = pieceType(toPiece);
    const int toColour = pieceColour(toPiece);

    if (fromType == PAWN || toPiece != EMPTY)
        board->halfMoveCounter = 0;
    else
        board->halfMoveCounter += 1;

    board->pieces[fromType]     ^= (1ull << from) ^ (1ull << to);
    board->colours[board->turn] ^= (1ull << from) ^ (1ull << to);

    board->pieces[toType]    ^= (1ull << to);
    board->colours[toColour] ^= (1ull << to);

    board->squares[from] = EMPTY;
    board->squares[to]   = fromPiece;
    undo->capturePiece   = toPiece;

    board->castleRooks &= board->castleMasks[from];
    board->castleRooks &= board->castleMasks[to];
    updateCastleZobrist(board, undo->castleRooks, board->castleRooks);

    board->psqtmat += PSQT[fromPiece][to]
                   -  PSQT[fromPiece][from]
                   -  PSQT[toPiece][to];

    board->hash    ^= ZobristKeys[fromPiece][from]
                   ^  ZobristKeys[fromPiece][to]
                   ^  ZobristKeys[toPiece][to]
                   ^  ZobristTurnKey;

    if (fromType == PAWN || fromType == KING)
        board->pkhash ^= ZobristKeys[fromPiece][from]
                      ^  ZobristKeys[fromPiece][to];

    if (toType == PAWN)
        board->pkhash ^= ZobristKeys[toPiece][to];

    if (fromType == PAWN && (to ^ from) == 16) {

        uint64_t enemyPawns =  board->pieces[PAWN]
                            &  board->colours[!board->turn]
                            &  adjacentFilesMasks(fileOf(from))
                            & (board->turn == WHITE ? RANK_4 : RANK_5);
        if (enemyPawns) {
            board->epSquare = board->turn == WHITE ? from + 8 : from - 8;
            board->hash ^= ZobristEnpassKeys[fileOf(from)];
        }
    }

    nnue_push(board);
    nnue_move_piece(board, fromPiece, from, to);
    nnue_remove_piece(board, toPiece, to);
}

void applyCastleMove(Board *board, uint16_t move, Undo *undo) {

    const int from = MoveFrom(move);
    const int rFrom = MoveTo(move);

    const int to = castleKingTo(from, rFrom);
    const int rTo = castleRookTo(from, rFrom);

    const int fromPiece = makePiece(KING, board->turn);
    const int rFromPiece = makePiece(ROOK, board->turn);

    board->halfMoveCounter += 1;

    board->pieces[KING]         ^= (1ull << from) ^ (1ull << to);
    board->colours[board->turn] ^= (1ull << from) ^ (1ull << to);

    board->pieces[ROOK]         ^= (1ull << rFrom) ^ (1ull << rTo);
    board->colours[board->turn] ^= (1ull << rFrom) ^ (1ull << rTo);

    board->squares[from]  = EMPTY;
    board->squares[rFrom] = EMPTY;

    board->squares[to]    = fromPiece;
    board->squares[rTo]   = rFromPiece;

    board->castleRooks &= board->castleMasks[from];
    updateCastleZobrist(board, undo->castleRooks, board->castleRooks);

    board->psqtmat += PSQT[fromPiece][to]
                   -  PSQT[fromPiece][from]
                   +  PSQT[rFromPiece][rTo]
                   -  PSQT[rFromPiece][rFrom];

    board->hash    ^= ZobristKeys[fromPiece][from]
                   ^  ZobristKeys[fromPiece][to]
                   ^  ZobristKeys[rFromPiece][rFrom]
                   ^  ZobristKeys[rFromPiece][rTo]
                   ^  ZobristTurnKey;

    board->pkhash  ^= ZobristKeys[fromPiece][from]
                   ^  ZobristKeys[fromPiece][to];

    assert(pieceType(fromPiece) == KING);

    undo->capturePiece = EMPTY;

    nnue_push(board);
    if (from != to) nnue_move_piece(board, fromPiece, from, to);
    if (rFrom != rTo) nnue_move_piece(board, rFromPiece, rFrom, rTo);
}

void applyEnpassMove(Board *board, uint16_t move, Undo *undo) {

    const int from = MoveFrom(move);
    const int to = MoveTo(move);
    const int ep = to - 8 + (board->turn << 4);

    const int fromPiece = makePiece(PAWN, board->turn);
    const int enpassPiece = makePiece(PAWN, !board->turn);

    board->halfMoveCounter = 0;

    board->pieces[PAWN]         ^= (1ull << from) ^ (1ull << to);
    board->colours[board->turn] ^= (1ull << from) ^ (1ull << to);

    board->pieces[PAWN]          ^= (1ull << ep);
    board->colours[!board->turn] ^= (1ull << ep);

    board->squares[from] = EMPTY;
    board->squares[to]   = fromPiece;
    board->squares[ep]   = EMPTY;
    undo->capturePiece   = enpassPiece;

    board->psqtmat += PSQT[fromPiece][to]
                   -  PSQT[fromPiece][from]
                   -  PSQT[enpassPiece][ep];

    board->hash    ^= ZobristKeys[fromPiece][from]
                   ^  ZobristKeys[fromPiece][to]
                   ^  ZobristKeys[enpassPiece][ep]
                   ^  ZobristTurnKey;

    board->pkhash  ^= ZobristKeys[fromPiece][from]
                   ^  ZobristKeys[fromPiece][to]
                   ^  ZobristKeys[enpassPiece][ep];

    assert(pieceType(fromPiece) == PAWN);
    assert(pieceType(enpassPiece) == PAWN);

    nnue_push(board);
    nnue_move_piece(board, fromPiece, from, to);
    nnue_remove_piece(board, enpassPiece, ep);
}

void applyPromotionMove(Board *board, uint16_t move, Undo *undo) {

    const int from = MoveFrom(move);
    const int to = MoveTo(move);

    const int fromPiece = board->squares[from];
    const int toPiece = board->squares[to];
    const int promoPiece = makePiece(MovePromoPiece(move), board->turn);

    const int toType = pieceType(toPiece);
    const int toColour = pieceColour(toPiece);
    const int promotype = MovePromoPiece(move);

    board->halfMoveCounter = 0;

    board->pieces[PAWN]         ^= (1ull << from);
    board->pieces[promotype]    ^= (1ull << to);
    board->colours[board->turn] ^= (1ull << from) ^ (1ull << to);

    board->pieces[toType]    ^= (1ull << to);
    board->colours[toColour] ^= (1ull << to);

    board->squares[from] = EMPTY;
    board->squares[to]   = promoPiece;
    undo->capturePiece   = toPiece;

    board->castleRooks &= board->castleMasks[to];
    updateCastleZobrist(board, undo->castleRooks, board->castleRooks);

    board->psqtmat += PSQT[promoPiece][to]
                   -  PSQT[fromPiece][from]
                   -  PSQT[toPiece][to];

    board->hash    ^= ZobristKeys[fromPiece][from]
                   ^  ZobristKeys[promoPiece][to]
                   ^  ZobristKeys[toPiece][to]
                   ^  ZobristTurnKey;

    board->pkhash  ^= ZobristKeys[fromPiece][from];

    assert(pieceType(fromPiece) == PAWN);
    assert(pieceType(toPiece) != PAWN);
    assert(pieceType(toPiece) != KING);

    nnue_push(board);
    nnue_remove_piece(board, fromPiece, from);
    nnue_remove_piece(board, toPiece, to);
    nnue_add_piece(board, promoPiece, to);
}

void applyNullMove(Board *board, Undo *undo) {

    // Save information which is hard to recompute
    undo->hash            = board->hash;
    undo->threats         = board->threats;
    undo->blockers        = board->blockers;
    undo->epSquare        = board->epSquare;
    undo->halfMoveCounter = board->halfMoveCounter++;

    // NULL moves simply swap the turn only
    board->turn = !board->turn;
    board->history[board->numMoves++] = board->hash;
    board->fullMoveCounter++;

    // Update the hash for turn and changes to enpass square
    board->hash ^= ZobristTurnKey;
    if (board->epSquare != -1) {
        board->hash ^= ZobristEnpassKeys[fileOf(board->epSquare)];
        board->epSquare = -1;
    }

    update_board_state(board);

    nnue_push(board);
}


void revert(Thread *thread, Board *board, uint16_t move) {

    if (move == NULL_MOVE)
        revertNullMove(board, &thread->undoStack[--thread->height]);
    else
        revertMove(board, move, &thread->undoStack[--thread->height]);
}

void revertMove(Board *board, uint16_t move, Undo *undo) {

    const int to = MoveTo(move);
    const int from = MoveFrom(move);

    // Revert information which is hard to recompute
    board->hash            = undo->hash;
    board->pkhash          = undo->pkhash;
    board->checkers        = undo->checkers;
    board->threats         = undo->threats;
    board->blockers        = undo->blockers;
    board->castleRooks     = undo->castleRooks;
    board->epSquare        = undo->epSquare;
    board->halfMoveCounter = undo->halfMoveCounter;
    board->psqtmat         = undo->psqtmat;

    // Swap turns and update the history index
    board->turn = !board->turn;
    board->numMoves--;
    board->fullMoveCounter--;

    if (MoveType(move) == NORMAL_MOVE) {

        const int fromType = pieceType(board->squares[to]);
        const int toType = pieceType(undo->capturePiece);
        const int toColour = pieceColour(undo->capturePiece);

        board->pieces[fromType]     ^= (1ull << from) ^ (1ull << to);
        board->colours[board->turn] ^= (1ull << from) ^ (1ull << to);

        board->pieces[toType]    ^= (1ull << to);
        board->colours[toColour] ^= (1ull << to);

        board->squares[from] = board->squares[to];
        board->squares[to] = undo->capturePiece;
    }

    else if (MoveType(move) == CASTLE_MOVE) {

        const int rFrom = to;
        const int rTo = castleRookTo(from, rFrom);
        const int _to = castleKingTo(from, rFrom);

        board->pieces[KING]         ^= (1ull << from) ^ (1ull << _to);
        board->colours[board->turn] ^= (1ull << from) ^ (1ull << _to);

        board->pieces[ROOK]         ^= (1ull << rFrom) ^ (1ull << rTo);
        board->colours[board->turn] ^= (1ull << rFrom) ^ (1ull << rTo);

        board->squares[_to] = EMPTY;
        board->squares[rTo] = EMPTY;

        board->squares[from] = makePiece(KING, board->turn);
        board->squares[rFrom] = makePiece(ROOK, board->turn);
    }

    else if (MoveType(move) == PROMOTION_MOVE) {

        const int toType = pieceType(undo->capturePiece);
        const int toColour = pieceColour(undo->capturePiece);
        const int promotype = MovePromoPiece(move);

        board->pieces[PAWN]         ^= (1ull << from);
        board->pieces[promotype]    ^= (1ull << to);
        board->colours[board->turn] ^= (1ull << from) ^ (1ull << to);

        board->pieces[toType]    ^= (1ull << to);
        board->colours[toColour] ^= (1ull << to);

        board->squares[from] = makePiece(PAWN, board->turn);
        board->squares[to] = undo->capturePiece;
    }

    else { // (MoveType(move) == ENPASS_MOVE)

        assert(MoveType(move) == ENPASS_MOVE);

        const int ep = to - 8 + (board->turn << 4);

        board->pieces[PAWN]         ^= (1ull << from) ^ (1ull << to);
        board->colours[board->turn] ^= (1ull << from) ^ (1ull << to);

        board->pieces[PAWN]          ^= (1ull << ep);
        board->colours[!board->turn] ^= (1ull << ep);

        board->squares[from] = board->squares[to];
        board->squares[to] = EMPTY;
        board->squares[ep] = undo->capturePiece;
    }
}

void revertNullMove(Board *board, Undo *undo) {

    // Revert information which is hard to recompute
    board->hash            = undo->hash;
    board->threats         = undo->threats;
    board->blockers        = undo->blockers;
    board->epSquare        = undo->epSquare;
    board->halfMoveCounter = undo->halfMoveCounter;

    // NULL moves simply swap the turn only
    board->turn = !board->turn;
    board->numMoves--;
    board->fullMoveCounter--;
}


int legalMoveCount(Board *board) {

    // Count of the legal number of moves for a given position

    uint16_t moves[MAX_MOVES];
    return genAllLegalMoves(board, moves);
}

int moveExaminedByMultiPV(Thread *thread, uint16_t move) {

    // Check to see if this move was already selected as the
    // best move in a previous iteration of this search depth

    for (int i = 0; i < thread->multiPV; i++)
        if (thread->bestMoves[i] == move)
            return 1;

    return 0;
}

int moveIsInRootMoves(Thread *thread, uint16_t move) {

    // We do two things: 1) Check to make sure we are not using a move which
    // has been flagged as excluded thanks to Syzygy probing. 2) Check to see
    // if we are doing a "go searchmoves <>"  command, in which case we have
    // to limit our search to the provided moves.

    for (int i = 0; i < MAX_MOVES; i++)
        if (move == thread->limits->excludedMoves[i])
            return 0;

    if (!thread->limits->limitedByMoves)
        return 1;

    for (int i = 0; i < MAX_MOVES; i++)
        if (move == thread->limits->searchMoves[i])
            return 1;

    return 0;
}

int moveIsTactical(Board *board, uint16_t move) {

    // We can use a simple bit trick since we assert that only
    // the enpass and promotion moves will ever have the 13th bit,
    // (ie 2 << 12) set. We use (2 << 12) in order to match move.h
    assert(ENPASS_MOVE & PROMOTION_MOVE & (2 << 12));
    assert(!((NORMAL_MOVE | CASTLE_MOVE) & (2 << 12)));

    // Check for captures, promotions, or enpassant. Castle moves may appear to be
    // tactical, since the King may move to its own square, or the rooks square
    return (board->squares[MoveTo(move)] != EMPTY && MoveType(move) != CASTLE_MOVE)
        || (move & ENPASS_MOVE & PROMOTION_MOVE);
}

int moveEstimatedValue(Board *board, uint16_t move) {

    // Start with the value of the piece on the target square
    int value = SEEPieceValues[pieceType(board->squares[MoveTo(move)])];

    // Factor in the new piece's value and remove our promoted pawn
    if (MoveType(move) == PROMOTION_MOVE)
        value += SEEPieceValues[MovePromoPiece(move)] - SEEPieceValues[PAWN];

    // Target square is encoded as empty for enpass moves
    else if (MoveType(move) == ENPASS_MOVE)
        value = SEEPieceValues[PAWN];

    // We encode Castle moves as KxR, so the initial step is wrong
    else if (MoveType(move) == CASTLE_MOVE)
        value = 0;

    return value;
}

int moveBestCaseValue(Board *board) {

    // Assume the opponent has at least a pawn
    int value = SEEPieceValues[PAWN];

    // Check for a higher value target
    for (int piece = QUEEN; piece > PAWN; piece--)
        if (board->pieces[piece] & board->colours[!board->turn])
          { value = SEEPieceValues[piece]; break; }

    // Check for a potential pawn promotion
    if (   board->pieces[PAWN]
        &  board->colours[board->turn]
        & (board->turn == WHITE ? RANK_7 : RANK_2))
        value += SEEPieceValues[QUEEN] - SEEPieceValues[PAWN];

    return value;
}


bool move_is_legal(Board *board, uint16_t move) {

    /// This function is predicated upon a few optimizations during movegen.
    /// #1. If there are two checkers, then only King moves should be attempted
    /// #2. All normal King moves will be legal during generation explicitly
    /// #3. If checked, only checker-captures and potential blockers will be generated
    /// #4. All castle moves will be legal, unless there is FRC implications
    /// #5. Enpass moves have no such stipulation and need to be done manually
    /// #6. Promotion moves are no different than regular moves regarding check

    const int from = MoveFrom(move), to = MoveTo(move);
    const int ksq = getlsb(board->pieces[KING] & board->colours[board->turn]);

    bool correct, actual;

    // {
    //     Undo undo[1];
    //     applyMove(board, move, undo);
    //     uint64_t occ = board->colours[WHITE] | board->colours[BLACK];
    //     const int k = getlsb(board->pieces[KING] & board->colours[!board->turn]);
    //     bool illeagl = allAttackersToSquare(board, occ, k) & board->colours[board->turn];
    //     revertMove(board, move, undo);
    //     correct = !illeagl;
    // }

    {

        if (MoveType(move) == ENPASS_MOVE) {

            Undo undo[1];
            applyMove(board, move, undo);
            uint64_t occ = board->colours[WHITE] | board->colours[BLACK];
            const int k = getlsb(board->pieces[KING] & board->colours[!board->turn]);
            bool illeagl = allAttackersToSquare(board, occ, k) & board->colours[board->turn];
            revertMove(board, move, undo);
            actual = !illeagl;

        }

        // Legal if not FRC, or if the Rook (to) is not a blocker
        else if (MoveType(move) == CASTLE_MOVE)
            actual = !board->chess960
                || !testBit(board->blockers, to);

        // Legal if not a blocker, or moving along the blocking path
        else
            actual = !testBit(board->blockers, from)
                  ||  testBit(attackRayMasks(ksq, from), to);

        return actual;
    }

    // if (correct != actual) {
    //     printBoard(board);
    //     printMove(move, board->chess960);
    //     printBitboard(board->checkers);
    //     printBitboard(board->threats);
    //     printBitboard(board->blockers);
    //     printBitboard(attackRayMasks(ksq, from));
    //     fflush(stdout);
    //     printf("%d %d\n", (int) correct, (int) actual);
    //     exit(EXIT_FAILURE);
    // }
    //
    // return correct;
}

bool move_is_pseudo_legal(Board *board, uint16_t move) {

    const int to    = MoveTo(move);
    const int from  = MoveFrom(move);
    const int type  = MoveType(move);
    const int ftype = pieceType(board->squares[from]);

    const uint64_t friendly = board->colours[ board->turn];
    const uint64_t enemy    = board->colours[!board->turn];
    const uint64_t occupied = friendly | enemy;

    // Skip obvious illegal moves, like our special ones, or those which attempt
    // to move a piece we don't control. Also, handle the weird but impossible case
    // of having the promotion bits set but not the promotion move-type flag
    if (    move == NONE_MOVE || move == NULL_MOVE
        ||  pieceColour(board->squares[from]) != board->turn
        || (MovePromoType(move) != PROMOTE_TO_KNIGHT && type != PROMOTION_MOVE))
        return FALSE;

    // In movegen, we filter all non-King moves when doubly checked
    if (several(board->checkers) && ftype != KING)
        return FALSE;

    // In movegen, we only generate fully legal castling moves
    if (ftype == KING && type == CASTLE_MOVE) {

        return  !board->checkers
            &&  !testBit(board->blockers, to)
            &&   move == MoveMake(from, to, CASTLE_MOVE)
            &&   testBit(friendly & board->castleRooks, to)
            && !(occupied & castle_occupied_mask(from, to))
            &&  !testBit(board->threats, castleKingTo(from, to))
            && !(bitsBetweenMasks(from, castleKingTo(from, to)) & board->threats);
    }

    // In movegen, Kings will only make completely legal moves
    if (ftype == KING)
        return type == NORMAL_MOVE
            && testBit(kingAttacks(from) & ~board->threats & ~friendly, to);

    // In movegen, non-pawn moves block or capture checkers when present
    if (ftype != PAWN) {

        // Block or capture the checking piece if there is one
        const uint64_t targets = !board->checkers ? ~0ULL
                               : (board->checkers | check_blocking_mask(board));
        const uint64_t attacks = pieceAttacks(ftype, board->turn, from, occupied);

        return type == NORMAL_MOVE && testBit(attacks & targets & ~friendly, to);
    }

    // In movegen, no special care is taken for Enpassant legality
    if (type == ENPASS_MOVE)
        return MoveTo(move) == board->epSquare
            && testBit(pawnAttacks(board->turn, from), to);

    else {

        // Block or capture the checking piece if there is one
        const uint64_t targets = !board->checkers ? ~0ULL
                               : (board->checkers | check_blocking_mask(board));
        const uint64_t attacks = pawnAttacks(board->turn, from);

        // Compute the possible single and double pushes
        const uint64_t rank3    = board->turn == WHITE ? RANK_3 : RANK_6;
        const uint64_t forward1 = pawnAdvance(1ULL << from, occupied, board->turn);
        const uint64_t forward2 = pawnAdvance(forward1 & rank3, occupied, board->turn);

        // Ensure that promotions, and only promotions, hit the final rank
        const uint64_t filter  = type == NORMAL_MOVE
                               ? ~PROMOTION_RANKS & targets
                               :  PROMOTION_RANKS & targets;

        // Tack on double pushes to the candiates if the move is normal
        const uint64_t refined = type == PROMOTION_MOVE
                               ? (attacks & enemy) | forward1
                               : (attacks & enemy) | forward1 | forward2;

        return type != CASTLE_MOVE && testBit(filter & refined, to);
    }
}



void printMove(uint16_t move, int chess960) {
    char str[6]; moveToString(move, str, chess960);
    printf("%s\n", str);
}

void moveToString(uint16_t move, char *str, int chess960) {

    int from = MoveFrom(move), to = MoveTo(move);

    // FRC reports using KxR notation, but standard does not
    if (MoveType(move) == CASTLE_MOVE && !chess960)
        to = castleKingTo(from, to);

    // Encode squares (Long Algebraic Notation)
    squareToString(from, &str[0]);
    squareToString(to, &str[2]);

    // Add promotion piece label (Uppercase)
    if (MoveType(move) == PROMOTION_MOVE) {
        str[4] = PieceLabel[BLACK][MovePromoPiece(move)];
        str[5] = '\0';
    }
}
