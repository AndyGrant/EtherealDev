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
#include <assert.h>

#include "board.h"
#include "bitboards.h"
#include "bitutils.h"
#include "castle.h"
#include "magics.h"
#include "masks.h"
#include "move.h"
#include "movegen.h"
#include "piece.h"
#include "types.h"

/* For Generating Attack BitBoards */

uint64_t pawnAttacks(int sq, uint64_t targets, int colour){
    return targets & (colour == WHITE ? ((((1ull << sq) << 7) & ~FILE_H) | (((1ull << sq) << 9) & ~FILE_A))
                                      : ((((1ull << sq) >> 7) & ~FILE_A) | (((1ull << sq) >> 9) & ~FILE_H)));
}

uint64_t knightAttacks(int sq, uint64_t targets){
    return KnightMap[sq] & targets;
}

uint64_t bishopAttacks(int sq, uint64_t occupied, uint64_t targets){
    uint64_t mask = occupied & OccupancyMaskBishop[sq];
    int index     = (mask * MagicNumberBishop[sq]) >> MagicShiftsBishop[sq];
    return MoveDatabaseBishop[MagicBishopIndexes[sq] + index] & targets;
}

uint64_t rookAttacks(int sq, uint64_t occupied, uint64_t targets){
    uint64_t mask = occupied & OccupancyMaskRook[sq];
    int index     = (mask * MagicNumberRook[sq]) >> MagicShiftsRook[sq];
    return MoveDatabaseRook[MagicRookIndexes[sq] + index] & targets;
}

uint64_t queenAttacks(int sq, uint64_t occupiedDiagonol, uint64_t occupiedStraight, uint64_t targets){
    return  bishopAttacks(sq, occupiedDiagonol, targets)
            | rookAttacks(sq, occupiedStraight, targets);
}

uint64_t kingAttacks(int sq, uint64_t targets){
    return KingMap[sq] & targets;
}


/* For Building Actual Move Lists For Each Piece Type */

void buildPawnMoves(uint16_t* moves, int* size, uint64_t attacks, int delta){
    while (attacks){
        int sq = poplsb(&attacks);
        moves[(*size)++] = MoveMake(sq + delta, sq, NORMAL_MOVE);
    }
}

void buildPawnPromotions(uint16_t* moves, int* size, uint64_t attacks, int delta){
    while (attacks){
        int sq = poplsb(&attacks);
        moves[(*size)++] = MoveMake(sq + delta, sq,  QUEEN_PROMO_MOVE);
        moves[(*size)++] = MoveMake(sq + delta, sq,   ROOK_PROMO_MOVE);
        moves[(*size)++] = MoveMake(sq + delta, sq, BISHOP_PROMO_MOVE);
        moves[(*size)++] = MoveMake(sq + delta, sq, KNIGHT_PROMO_MOVE);
    }
}

void buildNonPawnMoves(uint16_t* moves, int* size, uint64_t attacks, int sq){
    while (attacks){
        int tg = poplsb(&attacks);
        moves[(*size)++] = MoveMake(sq, tg, NORMAL_MOVE);
    }
}

void buildKnightMoves(uint16_t* moves, int* size, uint64_t pieces, uint64_t targets){
    while (pieces){
        int sq = poplsb(&pieces);
        buildNonPawnMoves(moves, size, knightAttacks(sq, targets), sq);
    }
}

void buildBishopAndQueenMoves(uint16_t* moves, int* size, uint64_t pieces, uint64_t occupied, uint64_t targets){
    while (pieces){
        int sq = poplsb(&pieces);
        buildNonPawnMoves(moves, size, bishopAttacks(sq, occupied, targets), sq);
    }
}

void buildRookAndQueenMoves(uint16_t* moves, int* size, uint64_t pieces, uint64_t occupied, uint64_t targets){
    while (pieces){
        int sq = poplsb(&pieces);
        buildNonPawnMoves(moves, size, rookAttacks(sq, occupied, targets), sq);
    }
}

void buildKingMoves(uint16_t* moves, int* size, uint64_t pieces, uint64_t targets){
    while (pieces){
        int sq = poplsb(&pieces);
        buildNonPawnMoves(moves, size, kingAttacks(sq, targets), sq);
    }
}


/* For Building Full Move Lists */

void genAllLegalMoves(Board* board, uint16_t* moves, int* size){
    
    int i, psuedoSize = 0;
    uint16_t psuedoMoves[MAX_MOVES];
    
    genAllMoves(board, psuedoMoves, &psuedoSize);
    
    // Check each move for legality before copying
    for (i = 0; i < psuedoSize; i++)
        if (moveIsLegal(board, psuedoMoves[i]))
            moves[(*size)++] = psuedoMoves[i];
}

void genAllMoves(Board* board, uint16_t* moves, int* size){
    
    int noisy = 0, quiet = 0;
    
    genAllNoisyMoves(board, moves, &noisy);
    
    genAllQuietMoves(board, moves + noisy, &quiet);
    
    *size = noisy + quiet;
}

void genAllNoisyMoves(Board* board, uint16_t* moves, int* size){
    
    uint64_t destinations;
    
    uint64_t pawnLeft;
    uint64_t pawnRight;
    
    uint64_t pawnPromoForward;
    uint64_t pawnPromoLeft;
    uint64_t pawnPromoRight;
    
    int forwardShift, leftShift, rightShift;
    int epSquare = board->epSquare;
    
    uint64_t friendly = board->colours[board->turn];
    uint64_t enemy = board->colours[!board->turn];
    
    uint64_t empty = ~(friendly | enemy);
    uint64_t notEmpty = ~empty;
    
    uint64_t myPawns   = friendly &  board->pieces[PAWN];
    uint64_t myKnights = friendly &  board->pieces[KNIGHT];
    uint64_t myBishops = friendly & (board->pieces[BISHOP] | board->pieces[QUEEN]);
    uint64_t myRooks   = friendly & (board->pieces[ROOK]   | board->pieces[QUEEN]);
    uint64_t myKings   = friendly &  board->pieces[KING];
    
    // If there are two threats to the king, the only moves
    // which could be legal are captures made by the king
    if (moreThanOne(board->kingAttackers)){
        buildKingMoves(moves, size, myKings, enemy);
        return;
    }
    
    // If there is one threat to the king and we are only looking for
    // noisy moves, then the only moves possible are captures of the
    // attacking piece, blocking the attacking piece via promotion,
    // and blocking the attacking piece via enpass. Therefore, we will
    // always generate all of the promotion and enpass moves
    else if (board->kingAttackers)
        destinations = board->kingAttackers;
    
    // No threats to the king, so any target square with an enemy piece,
    // or a move which is already noisy, is permitted in this position
    else
        destinations = enemy;
    
    // Generate Pawn BitBoards and Generate Enpass Moves
    if (board->turn == WHITE){
        forwardShift = -8;
        leftShift = -7;
        rightShift = -9;
        
        pawnLeft = ((myPawns << 7) & ~FILE_H) & enemy;
        pawnRight = ((myPawns << 9) & ~FILE_A) & enemy;
        
        pawnPromoForward = (myPawns << 8) & empty & RANK_8;
        pawnPromoLeft = pawnLeft & RANK_8;
        pawnPromoRight = pawnRight & RANK_8;
        
        pawnLeft &= ~RANK_8;
        pawnRight &= ~RANK_8;
        
        if(epSquare != -1){
            if (board->squares[epSquare-7] == WHITE_PAWN && epSquare != 47)
                moves[(*size)++] = MoveMake(epSquare-7, epSquare, ENPASS_MOVE);
            
            if (board->squares[epSquare-9] == WHITE_PAWN && epSquare != 40)
                moves[(*size)++] = MoveMake(epSquare-9, epSquare, ENPASS_MOVE);
        }
    } 
    
    else {
        forwardShift = 8;
        leftShift = 7;
        rightShift = 9;
        
        pawnLeft = ((myPawns >> 7) & ~FILE_A) & enemy;
        pawnRight = ((myPawns >> 9) & ~FILE_H) & enemy;
        
        pawnPromoForward = (myPawns >> 8) & empty & RANK_1;
        pawnPromoLeft = pawnLeft & RANK_1;
        pawnPromoRight = pawnRight & RANK_1;
        
        pawnLeft &= ~RANK_1;
        pawnRight &= ~RANK_1;
        
        if(epSquare != -1){
            if (board->squares[epSquare+7] == BLACK_PAWN && epSquare != 16)
                moves[(*size)++] = MoveMake(epSquare+7, epSquare, ENPASS_MOVE);
            
            if (board->squares[epSquare+9] == BLACK_PAWN && epSquare != 23)
                moves[(*size)++] = MoveMake(epSquare+9, epSquare, ENPASS_MOVE);
        }
    }
    
    // Generate all pawn captures that are not promotions
    buildPawnMoves(moves, size, pawnLeft & destinations, leftShift);
    buildPawnMoves(moves, size, pawnRight & destinations, rightShift);
    
    // Generate all pawn promotions
    buildPawnPromotions(moves, size, pawnPromoForward, forwardShift);
    buildPawnPromotions(moves, size, pawnPromoLeft, leftShift);
    buildPawnPromotions(moves, size, pawnPromoRight, rightShift);
    
    // Generate attacks for all non pawn pieces
    buildKnightMoves(moves, size, myKnights, destinations);
    buildBishopAndQueenMoves(moves, size, myBishops, notEmpty, destinations);
    buildRookAndQueenMoves(moves, size, myRooks, notEmpty, destinations);
    buildKingMoves(moves, size, myKings, enemy);
}

void genAllQuietMoves(Board* board, uint16_t* moves, int* size){
    
    uint64_t destinations;
    
    uint64_t pawnForwardOne;
    uint64_t pawnForwardTwo;
    
    uint64_t friendly = board->colours[board->turn];
    uint64_t enemy = board->colours[!board->turn];
    
    uint64_t empty = ~(friendly | enemy);
    uint64_t notEmpty = ~empty;
    
    uint64_t myPawns   = friendly &  board->pieces[PAWN];
    uint64_t myKnights = friendly &  board->pieces[KNIGHT];
    uint64_t myBishops = friendly & (board->pieces[BISHOP] | board->pieces[QUEEN]);
    uint64_t myRooks   = friendly & (board->pieces[ROOK]   | board->pieces[QUEEN]);
    uint64_t myKings   = friendly &  board->pieces[KING];
    
    // If there are two threats to the king, the only moves which
    // could be legal are moves made by the king, except castling
    if (moreThanOne(board->kingAttackers)){
        buildKingMoves(moves, size, myKings, empty);
        return;
    }
     
    // If there is one threat to the king there are two possible cases
    else if (board->kingAttackers){
        
        // If the king is attacked by a pawn or a knight, only moving the king
        // or capturing the pawn / knight will be legal. However, here we are
        // only generating quiet moves, thus we must move the king
        if (board->kingAttackers & (board->pieces[PAWN] | board->pieces[KNIGHT])){
            buildKingMoves(moves, size, myKings, empty);
            return;
        }
        
        // The attacker is a sliding piece, therefore we can either block the attack
        // by moving a piece infront of the attacking path if the slider, or we can
        // again simple move our king (Castling excluded, of course)
        destinations = empty & BitsBetweenMasks[getlsb(myKings)][getlsb(board->kingAttackers)];
    }
    
    // We are not being attacked, and therefore will look at any quiet move,
    // which means we can move to any square without a piece, again of course
    // with promotions and enpass excluded, as they are noisy moves
    else
        destinations = empty;
    
    
    // Generate the pawn advances
    if (board->turn == WHITE){
        pawnForwardOne = (myPawns << 8) & empty & ~RANK_8;
        pawnForwardTwo = ((pawnForwardOne & RANK_3) << 8) & empty;
        buildPawnMoves(moves, size, pawnForwardOne & destinations, -8);
        buildPawnMoves(moves, size, pawnForwardTwo & destinations, -16);
    } 
    
    else {
        pawnForwardOne = (myPawns >> 8) & empty & ~RANK_1;
        pawnForwardTwo = ((pawnForwardOne & RANK_6) >> 8) & empty;
        buildPawnMoves(moves, size, pawnForwardOne & destinations, 8);
        buildPawnMoves(moves, size, pawnForwardTwo & destinations, 16);
    }
    
    // Generate all moves for all non pawns aside from Castles
    buildKnightMoves(moves, size, myKnights, destinations);
    buildBishopAndQueenMoves(moves, size, myBishops, notEmpty, destinations);
    buildRookAndQueenMoves(moves, size, myRooks, notEmpty, destinations);
    buildKingMoves(moves, size, myKings, empty);
    
    // Generate all the castling moves
    if (board->turn == WHITE && !board->kingAttackers){
        
        if (  ((notEmpty & WHITE_CASTLE_KING_SIDE_MAP) == 0)
            && (board->castleRights & WHITE_KING_RIGHTS)
            && !squareIsAttacked(board, WHITE, 5))
            moves[(*size)++] = MoveMake(4, 6, CASTLE_MOVE);
            
        if (  ((notEmpty & WHITE_CASTLE_QUEEN_SIDE_MAP) == 0)
            && (board->castleRights & WHITE_QUEEN_RIGHTS)
            && !squareIsAttacked(board, WHITE, 3))
            moves[(*size)++] = MoveMake(4, 2, CASTLE_MOVE);
    }
    
    else if (board->turn == BLACK && !board->kingAttackers) {
        
        if (  ((notEmpty & BLACK_CASTLE_KING_SIDE_MAP) == 0)
            && (board->castleRights & BLACK_KING_RIGHTS)
            && !squareIsAttacked(board, BLACK, 61))
            moves[(*size)++] = MoveMake(60, 62, CASTLE_MOVE);
                     
        if (  ((notEmpty & BLACK_CASTLE_QUEEN_SIDE_MAP) == 0)
            && (board->castleRights & BLACK_QUEEN_RIGHTS)
            && !squareIsAttacked(board, BLACK, 59))
            moves[(*size)++] = MoveMake(60, 58, CASTLE_MOVE);
    }
}

int moveIsLegal(Board* board, uint16_t move){
    
    assert(moveIsPsuedoLegal(board, move));
    
    int sq, ep, legal;
    int to = MoveTo(move);
    int from = MoveFrom(move);
    int type = MoveType(move);
    int piece = PieceType(board->squares[from]);
    int pinned = !!(board->pinnedPieces & (1ull << from));
    
    uint64_t occupied, enemyBishops, enemyRooks;
    
    uint64_t friendly = board->colours[ board->turn];
    uint64_t enemy    = board->colours[!board->turn];
    
    uint64_t enemyPawns = enemy    & board->pieces[PAWN];
    uint64_t myKing     = friendly & board->pieces[KING];
    
    // If we are moving the King, all we must check is that the to square
    // is not under attack by any of our opponent's pieces.
    if (piece == KING){
        
        // Partially make the king move in order to use squareIsAttacked
        board->pieces[KING]         ^= (1ull << to) | (1ull << from);
        board->colours[board->turn] ^= (1ull << to) | (1ull << from);
        
        // See if our king can attack any enemy pieces from the new square
        legal = !squareIsAttacked(board, board->turn, to);
        
        // Unmake the partial king move so we are back where we started
        board->pieces[KING]         ^= (1ull << to) | (1ull << from);
        board->colours[board->turn] ^= (1ull << to) | (1ull << from);
        
        return legal;
    }
    
    // If we have more than one attacker, the only legal moves involve moving
    // our King, which was the very first condition we checked in moveIsLegal
    if (moreThanOne(board->kingAttackers))
        return 0;
    
    // If our king is not under threat, and we are not pinned, then we can
    // make any move other than enpassant moves, due to increased complexity
    if (!board->kingAttackers && !pinned){
        
        if (type != ENPASS_MOVE)
            return 1;
        
        sq = getlsb(myKing);
        ep = board->epSquare - 8 + (board->turn << 4);
        
        occupied = (enemy | friendly) ^ (1ull << from) ^ (1ull << to) ^ (1ull << ep);
        
        enemyBishops = enemy & (board->pieces[BISHOP] | board->pieces[QUEEN]);
        enemyRooks   = enemy & (board->pieces[ROOK  ] | board->pieces[QUEEN]);
        
        return   !bishopAttacks(sq, occupied, enemyBishops)
              &&   !rookAttacks(sq, occupied, enemyRooks  );
    }
    
    // If our king is not under threat but we are pinned, then we can make
    // any move so long as we stay within path between the king and the pinner
    if (!board->kingAttackers && pinned)
        return !!(LineBySquaresMasks[getlsb(myKing)][from] & (1ull << to));
    
    // We now can assume that we have exactly one threat on the king. Here,
    // the only pieces which can move are those which are not pinned (except
    // the King, of course). The only valid destination squares lie between
    // our King and the attacker, the square of the attacker, or in the case
    // of enpassant, the capture of the checking pawn.
    
    // If we are a non pinned piece and are capturing the attacker
    if (!pinned && (board->kingAttackers & (1ull << to)))
        return 1;
    
    // If we are a non pinned piece, and are moving in the line of the attacker
    if (!pinned && (BitsBetweenMasks[getlsb(myKing)][getlsb(board->kingAttackers)] & (1ull << to)))
        return 1;
    
    // If we are a non pinned pawn, and the attacking piece is an enemy pawn, then
    // the attacking piece must be the pawn which can be captured via enpassant
    if (!pinned && (board->kingAttackers & enemyPawns) && type == ENPASS_MOVE)
        return 1;
    
    // We have exahusted all possible ways to prove a move
    // legal, thus this move must be an illegal one.
    return 0;
}

int moveIsPsuedoLegal(Board* board, uint16_t move){
    
    int from, to, moveType, promoType, fromType; 
    int castleStart, castleEnd, rights, crossover;
    uint64_t friendly, enemy, empty, options, map;
    uint64_t forwardOne, forwardTwo, leftward, rightward;
    uint64_t promoForward, promoLeftward, promoRightward;
    
    if (move == NULL_MOVE || move == NONE_MOVE)
        return 0;
    
    from = MoveFrom(move);
    to = MoveTo(move);
    moveType = MoveType(move);
    promoType = MovePromoType(move);
    fromType = PieceType(board->squares[from]);
    friendly = board->colours[board->turn];
    enemy = board->colours[!board->turn];
    empty = ~(friendly | enemy);
    
    // Trying to move an empty square, or an enemy piece
    if (PieceColour(board->squares[from]) != board->turn)
        return 0;
    
    // Non promotion moves should be marked as Knight Promotions
    if (promoType != PROMOTE_TO_KNIGHT && moveType != PROMOTION_MOVE)
        return 0;
    
    switch (fromType){
        
        case PAWN:
        
            // Pawns cannot be involved in a Castle
            if (moveType == CASTLE_MOVE)
                return 0;
        
            // Compute bitboards for possible movement of a Pawn
            if (board->turn == WHITE){
                
                forwardOne     = ((1ull << from) << 8) & empty;
                forwardTwo     = ((forwardOne & RANK_3) << 8) & empty;
                leftward       = ((1ull << from) << 7) & ~FILE_H;
                rightward      = ((1ull << from) << 9) & ~FILE_A;
                promoForward   = forwardOne & RANK_8;
                promoLeftward  = leftward & RANK_8 & enemy;
                promoRightward = rightward & RANK_8 & enemy;
                forwardOne     = forwardOne & ~RANK_8;
                leftward       = leftward & ~RANK_8;
                rightward      = rightward & ~RANK_8;
                
            } else {
                
                forwardOne     = ((1ull << from) >> 8) & empty;
                forwardTwo     = ((forwardOne & RANK_6) >> 8) & empty;
                leftward       = ((1ull << from) >> 7) & ~FILE_A;
                rightward      = ((1ull << from) >> 9) & ~FILE_H;
                promoForward   = forwardOne & RANK_1;
                promoLeftward  = leftward & RANK_1 & enemy;
                promoRightward = rightward & RANK_1 & enemy;
                forwardOne     = forwardOne & ~RANK_1;
                leftward       = leftward & ~RANK_1;
                rightward      = rightward & ~RANK_1;
            }
            
            if (moveType == ENPASS_MOVE){
                
                // Make sure we can move to the enpass square
                if (!((leftward | rightward) & (1ull << board->epSquare))) return 0;
                
                // If the square matchs the to, then the move is valid
                return to == board->epSquare;
            }
            
            // Correct the movement bitboards
            leftward &= enemy; rightward &= enemy;
            
            // Determine the possible movements based on move type
            if (moveType == NORMAL_MOVE)
                options = forwardOne | forwardTwo | leftward | rightward;
            else
                options = promoForward | promoLeftward | promoRightward;
            
            // See if one of the possible moves includs to to square
            return (options & (1ull << to)) >> to;
                
        case KNIGHT:
            
            // First ensure the move type is correct
            if (moveType != NORMAL_MOVE) return 0;
            
            // Generate Knight attacks and compare to destination
            options = knightAttacks(from, ~friendly);
            return (options & (1ull << to)) >> to;
        
        
        case BISHOP:
            
            // First ensure the move type is correct
            if (moveType != NORMAL_MOVE) return 0;
        
            // Generate Bishop attacks and compare to destination
            options = bishopAttacks(from, ~empty, ~friendly);
            return (options & (1ull << to)) >> to;
        
        
        case ROOK:
        
            // First ensure the move type is correct
            if (moveType != NORMAL_MOVE) return 0;
            
            // Generate Rook attacks and compare to destination
            options = rookAttacks(from, ~empty, ~friendly);
            return (options & (1ull << to)) >> to;
        
        
        case QUEEN:
        
            // First ensure the move type is correct
            if (moveType != NORMAL_MOVE) return 0;
            
            // Generate Queen attacks and compare to destination
            options = bishopAttacks(from, ~empty, ~friendly)
                    | rookAttacks(from, ~empty, ~friendly);
            return (options & (1ull << to)) >> to;
        
        
        case KING:
        
            // If normal move, generate King attacks and compare to destination
            if (moveType == NORMAL_MOVE){
                options = kingAttacks(from, ~friendly);
                return (options & (1ull << to)) >> to;
            }
            
            else if (moveType == CASTLE_MOVE){
                
                // Determine the squares which must be unoccupied,
                // the needed castle rights, and the crossover square
                if (board->turn == WHITE){
                    map = (to > from)       ? WHITE_CASTLE_KING_SIDE_MAP : WHITE_CASTLE_QUEEN_SIDE_MAP;
                    rights = (to > from)    ? WHITE_KING_RIGHTS : WHITE_QUEEN_RIGHTS;
                    crossover = (to > from) ? 5 : 3;
                    castleEnd = (to > from) ? 6 : 2;
                    castleStart = 4;
                } 
                
                else {
                    map = (to > from)       ? BLACK_CASTLE_KING_SIDE_MAP : BLACK_CASTLE_QUEEN_SIDE_MAP;
                    rights = (to > from)    ? BLACK_KING_RIGHTS : BLACK_QUEEN_RIGHTS;
                    crossover = (to > from) ? 61 : 59;
                    castleEnd = (to > from) ? 62 : 58;
                    castleStart = 60;
                }
                
                // Inorder to be psuedo legal, the from square must
                // match the starting king square, the to square
                // must must the correct movement, the area between the king
                // and the rook must be empty, we must have the proper
                // castling rights, we must not current be in check,
                // and we must also not cross through a checked square.
                if (from != castleStart) return 0;
                if (to != castleEnd) return 0;
                if (~empty & map) return 0;
                if (!(board->castleRights & rights)) return 0;
                if (board->kingAttackers) return 0;
                return !squareIsAttacked(board, board->turn, crossover);
            }
            
            else
                return 0;
            
        default:
            assert(0);
            return 0;
    }
}

int squareIsAttacked(Board* board, int turn, int sq){
    
    uint64_t square;
    
    uint64_t friendly = board->colours[turn];
    uint64_t enemy = board->colours[!turn];
    uint64_t notEmpty = friendly | enemy;
    
    uint64_t enemyPawns   = enemy & board->pieces[PAWN];
    uint64_t enemyKnights = enemy & board->pieces[KNIGHT];
    uint64_t enemyBishops = enemy & board->pieces[BISHOP];
    uint64_t enemyRooks   = enemy & board->pieces[ROOK];
    uint64_t enemyQueens  = enemy & board->pieces[QUEEN];
    uint64_t enemyKings   = enemy & board->pieces[KING];
    
    enemyBishops |= enemyQueens;
    enemyRooks |= enemyQueens;
    square = (1ull << sq);
    
    // Pawns
    if (turn == WHITE){
        if (((square << 7 & ~FILE_H) | (square << 9 & ~FILE_A)) & enemyPawns)
            return 1;
    } else {
        if (((square >> 7 & ~FILE_A) | (square >> 9 & ~FILE_H)) & enemyPawns)
            return 1;
    }
    
    // Knights
    if (enemyKnights && knightAttacks(sq, enemyKnights)) return 1;
    
    // Bishops and Queens
    if (enemyBishops && bishopAttacks(sq, notEmpty, enemyBishops)) return 1;
    
    // Rooks and Queens
    if (enemyRooks && rookAttacks(sq, notEmpty, enemyRooks)) return 1;
    
    // King
    if (kingAttacks(sq, enemyKings)) return 1;
    
    return 0;
}

uint64_t attackersToSquare(Board* board, int colour, int sq){
    
    uint64_t friendly = board->colours[ colour];
    uint64_t enemy    = board->colours[!colour];
    uint64_t occupied = friendly | enemy;
    
    return      pawnAttacks(sq, enemy & board->pieces[PAWN  ], colour)
           |  knightAttacks(sq, enemy & board->pieces[KNIGHT])
           |  bishopAttacks(sq, occupied, enemy & (board->pieces[BISHOP] | board->pieces[QUEEN]))
           |    rookAttacks(sq, occupied, enemy & (board->pieces[ROOK  ] | board->pieces[QUEEN]))
           |    kingAttacks(sq, enemy & board->pieces[KING  ]);
}

uint64_t attackersToKingSquare(Board* board){
    int kingsq = getlsb(board->colours[board->turn] & board->pieces[KING]);
    return attackersToSquare(board, board->turn, kingsq);
}

uint64_t piecesPinnedToKingSquare(Board* board){
    
    uint64_t friendly = board->colours[ board->turn];
    uint64_t enemy    = board->colours[!board->turn];
    
    uint64_t bishops = board->pieces[BISHOP] | board->pieces[QUEEN];
    uint64_t rooks   = board->pieces[ROOK  ] | board->pieces[QUEEN];
    uint64_t kings   = board->pieces[KING  ];
    
    int sq, kingsq = getlsb(friendly & kings);
    
    uint64_t potentialPinners = bishopAttacks(kingsq, enemy, enemy & bishops)
                               |  rookAttacks(kingsq, enemy, enemy & rooks  );
                               
    uint64_t pinnedPieces = 0ull;
    
    while (potentialPinners){
        sq = poplsb(&potentialPinners);
        if (exactlyOne(BitsBetweenMasks[kingsq][sq] & friendly))
            pinnedPieces |= BitsBetweenMasks[kingsq][sq] & friendly;
    }
    
    return pinnedPieces;
}