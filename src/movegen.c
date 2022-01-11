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

#include "attacks.h"
#include "board.h"
#include "bitboards.h"
#include "masks.h"
#include "move.h"
#include "movegen.h"
#include "types.h"

typedef Bitboard (*JumperFunc)(Square);
typedef Bitboard (*SliderFunc)(Square, Bitboard);

Move* buildEnpassMoves(Move *moves, Bitboard attacks, Square epsq) {

    while (attacks)
        *(moves++) = MoveMake(poplsb(&attacks), epsq, ENPASS_MOVE);

    return moves;
}

Move* buildPawnMoves(Move *moves, Bitboard attacks, Square delta) {

    while (attacks) {
        Square sq = poplsb(&attacks);
        *(moves++) = MoveMake(sq + delta, sq, NORMAL_MOVE);
    }

    return moves;
}

Move* buildPawnPromotions(Move *moves, Bitboard attacks, Square delta) {

    while (attacks) {
        Square sq = poplsb(&attacks);
        *(moves++) = MoveMake(sq + delta, sq,  QUEEN_PROMO_MOVE);
        *(moves++) = MoveMake(sq + delta, sq,   ROOK_PROMO_MOVE);
        *(moves++) = MoveMake(sq + delta, sq, BISHOP_PROMO_MOVE);
        *(moves++) = MoveMake(sq + delta, sq, KNIGHT_PROMO_MOVE);
    }

    return moves;
}

Move* buildNormalMoves(Move *moves, Bitboard attacks, Square sq) {

    while (attacks)
        *(moves++) = MoveMake(sq, poplsb(&attacks), NORMAL_MOVE);

    return moves;
}

Move* buildJumperMoves(JumperFunc F, Move *moves, Bitboard pieces, Bitboard targets) {

    while (pieces) {
        Square sq = poplsb(&pieces);
        moves = buildNormalMoves(moves, F(sq) & targets, sq);
    }

    return moves;
}

Move* buildSliderMoves(SliderFunc F, Move *moves, Bitboard pieces, Bitboard targets, Bitboard occupied) {

    while (pieces) {
        Square sq = poplsb(&pieces);
        moves = buildNormalMoves(moves, F(sq, occupied) & targets, sq);
    }

    return moves;
}


int genAllLegalMoves(Board *board, Move *moves) {

    Undo undo[1];
    int size = 0, pseudo = 0;
    uint16_t pseudoMoves[MAX_MOVES];

    // Call genAllNoisyMoves() & genAllNoisyMoves()
    pseudo  = genAllNoisyMoves(board, pseudoMoves);
    pseudo += genAllQuietMoves(board, pseudoMoves + pseudo);

    // Check each move for legality before copying
    for (int i = 0; i < pseudo; i++) {
        applyMove(board, pseudoMoves[i], undo);
        if (moveWasLegal(board)) moves[size++] = pseudoMoves[i];
        revertMove(board, pseudoMoves[i], undo);
    }

    return size;
}

int genAllNoisyMoves(Board *board, Move *moves) {

    const Move *start = moves;

    Square Left    = board->turn == WHITE ? -7 : 7;
    Square Right   = board->turn == WHITE ? -9 : 9;
    Square Forward = board->turn == WHITE ? -8 : 8;

    Bitboard destinations, pawnEnpass, pawnLeft, pawnRight;
    Bitboard pawnPromoForward, pawnPromoLeft, pawnPromoRight;

    Bitboard occupied = board->get_pieces();
    Bitboard them     = board->get_pieces(!board->turn);

    Bitboard pawns    = board->get_pieces(board->turn,   PAWN       );
    Bitboard knights  = board->get_pieces(board->turn, KNIGHT       );
    Bitboard bishops  = board->get_pieces(board->turn, BISHOP, QUEEN);
    Bitboard rooks    = board->get_pieces(board->turn,   ROOK, QUEEN);
    Bitboard kings    = board->get_pieces(board->turn,   KING       );

    // Double checks can only be evaded by moving the King
    if (several(board->kingAttackers))
        return buildJumperMoves(&kingAttacks, moves, kings, them) - start;

    // When checked, we may only uncheck by capturing the checker
    destinations = board->kingAttackers ? board->kingAttackers : them;

    // Compute bitboards for each type of Pawn movement
    pawnEnpass       = pawnEnpassCaptures(pawns, board->epSquare, board->turn);
    pawnLeft         = pawnLeftAttacks(pawns, them, board->turn);
    pawnRight        = pawnRightAttacks(pawns, them, board->turn);
    pawnPromoForward = pawnAdvance(pawns, occupied, board->turn) & PROMOTION_RANKS;
    pawnPromoLeft    = pawnLeft  & PROMOTION_RANKS; pawnLeft  &= ~PROMOTION_RANKS;
    pawnPromoRight   = pawnRight & PROMOTION_RANKS; pawnRight &= ~PROMOTION_RANKS;

    // Generate moves for all the Pawns, so long as they are noisy
    moves = buildEnpassMoves(moves, pawnEnpass, board->epSquare);
    moves = buildPawnMoves(moves, pawnLeft & destinations, Left);
    moves = buildPawnMoves(moves, pawnRight & destinations, Right);
    moves = buildPawnPromotions(moves, pawnPromoForward, Forward);
    moves = buildPawnPromotions(moves, pawnPromoLeft, Left);
    moves = buildPawnPromotions(moves, pawnPromoRight, Right);

    // Generate moves for the remainder of the pieces, so long as they are noisy
    moves = buildJumperMoves(&knightAttacks, moves, knights, destinations);
    moves = buildSliderMoves(&bishopAttacks, moves, bishops, destinations, occupied);
    moves = buildSliderMoves(&rookAttacks, moves, rooks, destinations, occupied);
    moves = buildJumperMoves(&kingAttacks, moves, kings, them);

    return moves - start;
}

int genAllQuietMoves(Board *board, Move *moves) {

    const uint16_t *start = moves;

    const int Forward = board->turn == WHITE ? -8 : 8;
    const uint64_t Rank3Relative = board->turn == WHITE ? RANK_3 : RANK_6;

    int rook, king, rookTo, kingTo, attacked;
    uint64_t destinations, pawnForwardOne, pawnForwardTwo, mask;

    uint64_t occupied = board->get_pieces();
    uint64_t them     = board->get_pieces(!board->turn);
    uint64_t castles  = board->castleRooks & ~them;

    uint64_t pawns    = board->get_pieces(board->turn,   PAWN       );
    uint64_t knights  = board->get_pieces(board->turn, KNIGHT       );
    uint64_t bishops  = board->get_pieces(board->turn, BISHOP, QUEEN);
    uint64_t rooks    = board->get_pieces(board->turn,   ROOK, QUEEN);
    uint64_t kings    = board->get_pieces(board->turn,   KING       );

    // Double checks can only be evaded by moving the King
    if (several(board->kingAttackers))
        return buildJumperMoves(&kingAttacks, moves, kings, ~occupied) - start;

    // When checked, we must block the checker with non-King pieces
    destinations = !board->kingAttackers ? ~occupied
                 : bitsBetweenMasks(getlsb(kings), getlsb(board->kingAttackers));

    // Compute bitboards for each type of Pawn movement
    pawnForwardOne = pawnAdvance(pawns, occupied, board->turn) & ~PROMOTION_RANKS;
    pawnForwardTwo = pawnAdvance(pawnForwardOne & Rank3Relative, occupied, board->turn);

    // Generate moves for all the pawns, so long as they are quiet
    moves = buildPawnMoves(moves, pawnForwardOne & destinations, Forward);
    moves = buildPawnMoves(moves, pawnForwardTwo & destinations, Forward * 2);

    // Generate moves for the remainder of the pieces, so long as they are quiet
    moves = buildJumperMoves(&knightAttacks, moves, knights, destinations);
    moves = buildSliderMoves(&bishopAttacks, moves, bishops, destinations, occupied);
    moves = buildSliderMoves(&rookAttacks, moves, rooks, destinations, occupied);
    moves = buildJumperMoves(&kingAttacks, moves, kings, ~occupied);

    // Attempt to generate a castle move for each rook
    while (castles && !board->kingAttackers) {

        // Figure out which pieces are moving to which squares
        rook = poplsb(&castles), king = getlsb(kings);
        rookTo = castleRookTo(king, rook);
        kingTo = castleKingTo(king, rook);
        attacked = 0;

        // Castle is illegal if we would go over a piece
        mask  = bitsBetweenMasks(king, kingTo) | (1ull << kingTo);
        mask |= bitsBetweenMasks(rook, rookTo) | (1ull << rookTo);
        mask &= ~((1ull << king) | (1ull << rook));
        if (occupied & mask) continue;

        // Castle is illegal if we move through a checking threat
        mask = bitsBetweenMasks(king, kingTo);
        while (mask)
            if (squareIsAttacked(board, board->turn, poplsb(&mask)))
                { attacked = 1; break; }
        if (attacked) continue;

        // All conditions have been met. Identify which side we are castling to
        *(moves++) = MoveMake(king, rook, CASTLE_MOVE);
    }

    return moves - start;
}
