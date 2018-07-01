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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "attacks.h"
#include "board.h"
#include "bitboards.h"
#include "castle.h"
#include "evaluate.h"
#include "masks.h"
#include "movegen.h"
#include "psqt.h"
#include "transposition.h"
#include "types.h"

#ifdef TUNE
    const int TEXEL = 1;
    const int TRACE = 1;
    const EvalTrace EmptyTrace;
    EvalTrace T;
#else
    const int TEXEL = 0;
    const int TRACE = 0;
    EvalTrace T;
#endif

#define S(mg, eg) (MakeScore((mg), (eg)))

/* Material Value Evaluation Terms */

const int PawnValue   = S( 100, 123);
const int KnightValue = S( 463, 392);
const int BishopValue = S( 473, 417);
const int RookValue   = S( 639, 717);
const int QueenValue  = S(1313,1348);
const int KingValue   = S(   0,   0);

const int PieceValues[8][PHASE_NB] = {
    { 100, 123}, { 463, 392}, { 473, 417}, { 639, 717},
    {1313,1348}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int PawnIsolated = S(  10, -14);
const int PawnStacked = S( -14, -29);
const int PawnBackwards[2] = {
    S(   5,   1), S( -10, -28),
};
const int PawnConnected32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(  11, -12), S(  10,  15), S(   6,   5), S(  15,  24),
    S(  23,   3), S(  45,   1), S(  35,  10), S(  41,  33),
    S(  15,   3), S(  36,   4), S(  23,   7), S(  35,  26),
    S(  17,   9), S(  31,  21), S(  35,  22), S(  53,  23),
    S(  24,  37), S(  27,  49), S(  73,  61), S(  54,  77),
    S( 105, -16), S( 198,  12), S( 226,  18), S( 250,  74),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightOutpost[2] = {
    S(  28, -21), S(  47,  13),
};
const int KnightMobility[9] = {
    S( -88, -97), S( -50, -95), S( -34, -53), S( -14, -29),
    S(   0, -29), S(   7,  -4), S(  22,  -2), S(  39,   0),
    S(  60, -62),
};
const int BishopPair = S(  50,  80);
const int BishopRammedPawns = S( -17, -20);
const int BishopOutpost[2] = {
    S(  32,  -7), S(  54,   9),
};
const int BishopMobility[14] = {
    S( -60,-128), S( -59, -70), S( -21, -54), S(  -9, -31),
    S(   7, -18), S(  20,  -2), S(  26,  10), S(  32,   3),
    S(  35,  17), S(  45,   3), S(  39,  11), S(  44, -28),
    S(  47,  -1), S(  29, -76),
};
const int RookFile[2] = {
    S(  19, -16), S(  50,  -5),
};
const int RookOnSeventh = S(   9,  43);
const int RookMobility[15] = {
    S(-149,-107), S( -81,-122), S( -28, -76), S( -12, -35),
    S(  -9,  -9), S( -10,  11), S( -10,  23), S(  -1,  34),
    S(   6,  45), S(  15,  55), S(  24,  56), S(  35,  65),
    S(  36,  65), S(  40,  49), S(  29,  27),
};
const int QueenMobility[28] = {
    S( -60,-262), S(-216,-390), S( -48,-205), S( -44,-190),
    S( -20,-127), S( -32, -67), S( -17, -92), S( -21, -85),
    S( -17, -61), S( -11, -56), S(  -8, -32), S(  -3, -27),
    S(  -1, -15), S(   4,  -8), S(   5,  -3), S(   3,   6),
    S(   8,  19), S(   5,  18), S(  16,  28), S(   0,  23),
    S(   4,  24), S(  17,  25), S(  12,   6), S(  38,  11),
    S(  43,  19), S(  61,   0), S( -33,  -7), S(  13,   2),
};
const int KingDefenders[12] = {
    S( -40,  25), S( -20,  -9), S(  -2,  -7), S(   8,  -1),
    S(  25,  -6), S(  43,   1), S(  39,  13), S(  28,-206),
    S(  12,   6), S(  12,   6), S(  12,   6), S(  12,   6),
};
const int KingShelter[2][8][8] = {
  {{S( -17,  12), S(  18, -16), S(  22,   0), S(  22,   0), S(   8,   7), S(  30,   3), S(   0, -32), S( -36,  20)},
   {S(   2,   7), S(  26,  -8), S(  20,  -7), S(  -2, -14), S( -26,   0), S( -66,  79), S( 100,  93), S( -31,   6)},
   {S(  11,  11), S(  14,  -4), S( -23,   3), S(  -6,  -2), S( -21,  -2), S(  14,  -9), S(   4,  64), S( -15,   2)},
   {S(   8,  34), S(  18,  -3), S( -12,  -9), S(  30, -13), S(  25, -34), S( -21, -32), S(-147,  41), S(   0,  -4)},
   {S( -19,  24), S(  -5,   7), S( -38,   2), S( -15,   0), S( -25, -18), S( -41, -19), S(  39,  -8), S( -19,  -2)},
   {S(  22,   3), S(  26,  -6), S( -19,   0), S( -16, -22), S(  10, -32), S(  19, -47), S(  50, -33), S( -20,   2)},
   {S(  17,  -3), S(  -1, -15), S( -37,  -6), S( -21, -16), S( -25, -21), S( -43,   9), S(   7,  47), S( -22,  14)},
   {S( -20,  -7), S(   0, -13), S(  15,   2), S(  11,   6), S( -18,  12), S(   4,  35), S(-200,  79), S( -33,  20)}},
  {{S(   0,   0), S(   2, -20), S(   6, -13), S( -46,  19), S(   0,  -8), S(   9,  51), S(-170, -15), S( -54,  17)},
   {S(   0,   0), S(  27, -11), S(  13,  -7), S(  -5,  -7), S(  11, -32), S(  51, 102), S(-188, -11), S( -46,  15)},
   {S(   0,   0), S(  37,   0), S(   0,  -8), S(  24, -25), S(  14,  -7), S(-114,  44), S( -90, -84), S( -19,   1)},
   {S(   0,   0), S(  -7,   6), S( -13,  18), S( -14,   1), S( -23,   0), S(-116,  10), S(   4, -49), S( -33,  -8)},
   {S(   0,   0), S(   0,   7), S(   0,  -6), S(  30,  -6), S(  16, -23), S( -44,   8), S(-108, -72), S(  -8,  -7)},
   {S(   0,   0), S(   1,   1), S( -18,   0), S( -12, -13), S(  30, -37), S( -45,   5), S(  59,  47), S( -32,   4)},
   {S(   0,   0), S(  19, -11), S(  -1,   0), S( -21,  -4), S( -23, -13), S(  23,  12), S( -57, -63), S( -49,  27)},
   {S(   0,   0), S(   7, -37), S(  16, -16), S( -24,  -1), S( -26,  -1), S(   7, -16), S(-239, -73), S( -51,  12)}},
};
const int PassedPawn[2][2][8] = {
  {{S(   0,   0), S( -32, -27), S( -25,   4), S( -17, -12), S(  29, -16), S(  64, -16), S( 152,  31), S(   0,   0)},
   {S(   0,   0), S(   0,   7), S( -17,  28), S( -11,  39), S(  19,  32), S(  76,  51), S( 195, 119), S(   0,   0)}},
  {{S(   0,   0), S(  -1,  19), S( -10,   2), S(  -1,  11), S(  48,  12), S(  95,  40), S( 236, 117), S(   0,   0)},
   {S(   0,   0), S(  -1,  29), S(  -1,  36), S( -11,  73), S(   5, 138), S(  50, 234), S( 125, 377), S(   0,   0)}},
};
const int ThreatPawnAttackedByOne = S(  -8, -35);
const int ThreatMinorAttackedByPawn = S( -64, -52);
const int ThreatMinorAttackedByMajor = S( -33, -38);
const int ThreatQueenAttackedByOne = S( -76,   4);
const int ThreatOverloadedPieces = S(  -6, -26);
const int ThreatByPawnPush = S(  15,  14);


/* King Safety Evaluation Terms */

const int KSAttackWeight[]  = { 0, 16, 6, 10, 8, 0 };
const int KSAttackValue     =   44;
const int KSWeakSquares     =   38;
const int KSFriendlyPawns   =  -22;
const int KSNoEnemyQueens   = -256;
const int KSSafeQueenCheck  =   95;
const int KSSafeRookCheck   =   94;
const int KSSafeBishopCheck =   51;
const int KSSafeKnightCheck =  123;
const int KSAdjustment      =  -38;

/* Passed Pawn Evaluation Terms */


/* General Evaluation Terms */

const int Tempo[COLOUR_NB] = { S(  25,  12), S( -25, -12) };

#undef S


int evaluateBoard(Board* board, PawnKingTable* pktable){

    EvalInfo ei; // Fix bogus GCC warning
    ei.pkeval[WHITE] = ei.pkeval[BLACK] = 0;

    int phase, eval, pkeval;

    // Check for obviously drawn positions
    if (evaluateDraws(board)) return 0;

    // Setup and perform the evaluation of all pieces
    initializeEvalInfo(&ei, board, pktable);
    eval = evaluatePieces(&ei, board);

    // Store a new Pawn King Entry if we did not have one
    if (ei.pkentry == NULL && !TEXEL){
        pkeval = ei.pkeval[WHITE] - ei.pkeval[BLACK];
        storePawnKingEntry(pktable, board->pkhash, ei.passedPawns, pkeval);
    }

    // Add in the PSQT and Material values, as well as the tempo
    eval += board->psqtmat + Tempo[board->turn];

    // Calcuate the game phase based on remaining material (Fruit Method)
    phase = 24 - popcount(board->pieces[QUEEN]) * 4
               - popcount(board->pieces[ROOK]) * 2
               - popcount(board->pieces[KNIGHT] | board->pieces[BISHOP]);
    phase = (phase * 256 + 12) / 24;

    // Compute the interpolated evaluation
    eval  = (ScoreMG(eval) * (256 - phase) + ScoreEG(eval) * phase) / 256;

    // Return the evaluation relative to the side to move
    return board->turn == WHITE ? eval : -eval;
}

int evaluateDraws(Board* board){

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];
    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t kings   = board->pieces[KING];

    // Unlikely to have a draw if we have pawns, rooks, or queens left
    if (board->pieces[PAWN] | board->pieces[ROOK] | board->pieces[QUEEN])
        return 0;

    // Check for King Vs. King
    if (kings == (white | black)) return 1;

    if ((white & kings) == white){
        // Check for King Vs King and Knight/Bishop
        if (popcount(black & (knights | bishops)) <= 1) return 1;

        // Check for King Vs King and two Knights
        if (popcount(black & knights) == 2 && (black & bishops) == 0ull) return 1;
    }

    if ((black & kings) == black){
        // Check for King Vs King and Knight/Bishop
        if (popcount(white & (knights | bishops)) <= 1) return 1;

        // Check for King Vs King and two Knights
        if (popcount(white & knights) == 2 && (white & bishops) == 0ull) return 1;
    }

    return 0;
}

int evaluatePieces(EvalInfo *ei, Board *board) {

    int eval = 0;

    eval += evaluatePawns(ei, board, WHITE)
          - evaluatePawns(ei, board, BLACK);

    eval += evaluateKnights(ei, board, WHITE)
          - evaluateKnights(ei, board, BLACK);

    eval += evaluateBishops(ei, board, WHITE)
          - evaluateBishops(ei, board, BLACK);

    eval += evaluateRooks(ei, board, WHITE)
          - evaluateRooks(ei, board, BLACK);

    eval += evaluateQueens(ei, board, WHITE)
          - evaluateQueens(ei, board, BLACK);

    eval += evaluateKings(ei, board, WHITE)
          - evaluateKings(ei, board, BLACK);

    eval += evaluatePassedPawns(ei, board, WHITE)
          - evaluatePassedPawns(ei, board, BLACK);

    eval += evaluateThreats(ei, board, WHITE)
          - evaluateThreats(ei, board, BLACK);

    return eval;
}

int evaluatePawns(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const int Forward = (colour == WHITE) ? 8 : -8;

    int sq, semi, eval = 0;
    uint64_t pawns, myPawns, tempPawns, enemyPawns, attacks;

    // Store off pawn attacks for king safety and threat computations
    ei->attackedBy2[US]      = ei->pawnAttacks[US] & ei->attacked[US];
    ei->attacked[US]        |= ei->pawnAttacks[US];
    ei->attackedBy[US][PAWN] = ei->pawnAttacks[US];

    // Update attacker counts for King Safety computation
    attacks = ei->pawnAttacks[US] & ei->kingAreas[THEM];
    ei->kingAttacksCount[US] += popcount(attacks);

    // Pawn hash holds the pawn evaluation for both colours from white's POV
    if (ei->pkentry != NULL) return US == WHITE ? ei->pkentry->eval : 0;

    pawns = board->pieces[PAWN];
    myPawns = tempPawns = pawns & board->colours[US];
    enemyPawns = pawns & board->colours[THEM];

    // Evaluate each pawn (but not for being passed)
    while (tempPawns) {

        // Pop off the next pawn
        sq = poplsb(&tempPawns);
        if (TRACE) T.PawnValue[US]++;
        if (TRACE) T.PawnPSQT32[relativeSquare32(sq, US)][US]++;

        // Save passed pawn information for later evaluation
        if (!(passedPawnMasks(US, sq) & enemyPawns))
            setBit(&ei->passedPawns, sq);

        // Apply a penalty if the pawn is isolated
        if (!(isolatedPawnMasks(sq) & tempPawns)) {
            eval += PawnIsolated;
            if (TRACE) T.PawnIsolated[US]++;
        }

        // Apply a penalty if the pawn is stacked
        if (Files[fileOf(sq)] & tempPawns) {
            eval += PawnStacked;
            if (TRACE) T.PawnStacked[US]++;
        }

        // Apply a penalty if the pawn is backward
        if (   !(passedPawnMasks(THEM, sq) & myPawns)
            &&  (testBit(ei->pawnAttacks[THEM], sq + Forward))) {
            semi = !(Files[fileOf(sq)] & enemyPawns);
            eval += PawnBackwards[semi];
            if (TRACE) T.PawnBackwards[semi][US]++;
        }

        // Apply a bonus if the pawn is connected and not backward
        else if (pawnConnectedMasks(US, sq) & myPawns) {
            eval += PawnConnected32[relativeSquare32(sq, US)];
            if (TRACE) T.PawnConnected32[relativeSquare32(sq, US)][US]++;
        }
    }

    ei->pkeval[US] = eval; // Save eval for the Pawn Hash

    return eval;
}

int evaluateKnights(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, defended, count, eval = 0;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempKnights = board->pieces[KNIGHT] & board->colours[US  ];

    ei->attackedBy[US][KNIGHT] = 0ull;

    // Evaluate each knight
    while (tempKnights) {

        // Pop off the next knight
        sq = poplsb(&tempKnights);
        if (TRACE) T.KnightValue[US]++;
        if (TRACE) T.KnightPSQT32[relativeSquare32(sq, US)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = knightAttacks(sq);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][KNIGHT] |= attacks;

        // Apply a bonus if the knight is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the knight
        if (     testBit(outpostRanks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += KnightOutpost[defended];
            if (TRACE) T.KnightOutpost[defended][US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += KnightMobility[count];
        if (TRACE) T.KnightMobility[count][US]++;

        // Update for King Safety calculation
        attacks = attacks & ei->kingAreas[THEM];
        if (attacks) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[KNIGHT];
        }
    }

    return eval;
}

int evaluateBishops(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, defended, count, eval = 0;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempBishops = board->pieces[BISHOP] & board->colours[US  ];

    ei->attackedBy[US][BISHOP] = 0ull;

    // Apply a bonus for having a pair of bishops
    if ((tempBishops & WHITE_SQUARES) && (tempBishops & BLACK_SQUARES)) {
        eval += BishopPair;
        if (TRACE) T.BishopPair[US]++;
    }

    // Evaluate each bishop
    while (tempBishops) {

        // Pop off the next Bishop
        sq = poplsb(&tempBishops);
        if (TRACE) T.BishopValue[US]++;
        if (TRACE) T.BishopPSQT32[relativeSquare32(sq, US)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = bishopAttacks(sq, ei->occupiedMinusBishops[US]);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][BISHOP] |= attacks;

        // Apply a penalty for the bishop based on number of rammed pawns
        // of our own colour, which reside on the same shade of square as the bishop
        count = popcount(ei->rammedPawns[US] & (testBit(WHITE_SQUARES, sq) ? WHITE_SQUARES : BLACK_SQUARES));
        eval += count * BishopRammedPawns;
        if (TRACE) T.BishopRammedPawns[US] += count;

        // Apply a bonus if the bishop is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the bishop.
        if (     testBit(outpostRanks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += BishopOutpost[defended];
            if (TRACE) T.BishopOutpost[defended][US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += BishopMobility[count];
        if (TRACE) T.BishopMobility[count][US]++;

        // Update for King Safety calculation
        attacks = attacks & ei->kingAreas[THEM];
        if (attacks) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[BISHOP];
        }
    }

    return eval;
}

int evaluateRooks(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, open, count, eval = 0;
    uint64_t attacks;

    uint64_t myPawns    = board->pieces[PAWN] & board->colours[  US];
    uint64_t enemyPawns = board->pieces[PAWN] & board->colours[THEM];
    uint64_t tempRooks  = board->pieces[ROOK] & board->colours[  US];
    uint64_t enemyKings = board->pieces[KING] & board->colours[THEM];

    ei->attackedBy[US][ROOK] = 0ull;

    // Evaluate each rook
    while (tempRooks) {

        // Pop off the next rook
        sq = poplsb(&tempRooks);
        if (TRACE) T.RookValue[US]++;
        if (TRACE) T.RookPSQT32[relativeSquare32(sq, US)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[US]);
        ei->attackedBy2[US]      |= attacks & ei->attacked[US];
        ei->attacked[US]         |= attacks;
        ei->attackedBy[US][ROOK] |= attacks;

        // Rook is on a semi-open file if there are no pawns of the rook's
        // colour on the file. If there are no pawns at all, it is an open file
        if (!(myPawns & Files[fileOf(sq)])) {
            open = !(enemyPawns & Files[fileOf(sq)]);
            eval += RookFile[open];
            if (TRACE) T.RookFile[open][US]++;
        }

        // Rook gains a bonus for being located on seventh rank relative to its
        // colour so long as the enemy king is on the last two ranks of the board
        if (   relativeRankOf(US, sq) == 6
            && relativeRankOf(US, getlsb(enemyKings)) >= 6) {
            eval += RookOnSeventh;
            if (TRACE) T.RookOnSeventh[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the rook
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += RookMobility[count];
        if (TRACE) T.RookMobility[count][US]++;

        // Update for King Safety calculation
        attacks = attacks & ei->kingAreas[THEM];
        if (attacks) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[ROOK];
        }
    }

    return eval;
}

int evaluateQueens(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, count, eval = 0;
    uint64_t tempQueens, attacks;

    tempQueens = board->pieces[QUEEN] & board->colours[US];

    ei->attackedBy[US][QUEEN] = 0ull;

    // Evaluate each queen
    while (tempQueens) {

        // Pop off the next queen
        sq = poplsb(&tempQueens);
        if (TRACE) T.QueenValue[US]++;
        if (TRACE) T.QueenPSQT32[relativeSquare32(sq, US)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[US])
                | bishopAttacks(sq, ei->occupiedMinusBishops[US]);
        ei->attackedBy2[US]       |= attacks & ei->attacked[US];
        ei->attacked[US]          |= attacks;
        ei->attackedBy[US][QUEEN] |= attacks;

        // Apply a bonus (or penalty) based on the mobility of the queen
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += QueenMobility[count];
        if (TRACE) T.QueenMobility[count][US]++;

        // Update for King Safety calculation
        attacks = attacks & ei->kingAreas[THEM];
        if (attacks) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[QUEEN];
        }
    }

    return eval;
}

int evaluateKings(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int count, eval = 0, pkeval = 0;

    uint64_t myPawns     = board->pieces[PAWN ] & board->colours[  US];
    uint64_t enemyQueens = board->pieces[QUEEN] & board->colours[THEM];
    uint64_t myKings     = board->pieces[KING ] & board->colours[  US];

    uint64_t myDefenders  = (board->pieces[PAWN  ] & board->colours[US])
                          | (board->pieces[KNIGHT] & board->colours[US])
                          | (board->pieces[BISHOP] & board->colours[US]);

    int kingSq = getlsb(myKings);
    int kingFile = fileOf(kingSq);
    int kingRank = rankOf(kingSq);

    if (TRACE) T.KingValue[US]++;
    if (TRACE) T.KingPSQT32[relativeSquare32(kingSq, US)][US]++;

    // Bonus for our pawns and minors sitting within our king area
    count = popcount(myDefenders & ei->kingAreas[US]);
    eval += KingDefenders[count];
    if (TRACE) T.KingDefenders[count][US]++;

    // Perform King Safety when we have two attackers, or
    // one attacker with a potential for a Queen attacker
    if (ei->kingAttackersCount[THEM] > 1 - popcount(enemyQueens)) {

        // Weak squares are attacked by the enemy, defended no more
        // than once and only defended by our Queens or our King
        uint64_t weak =   ei->attacked[THEM]
                      &  ~ei->attackedBy2[US]
                      & (~ei->attacked[US] | ei->attackedBy[US][QUEEN] | ei->attackedBy[US][KING]);

        // Usually the King Area is 9 squares. Scale are attack counts to account for
        // when the king is in an open area and expects more attacks, or the opposite
        float scaledAttackCounts = 9.0 * ei->kingAttacksCount[THEM] / popcount(ei->kingAreas[US]);

        // Safe target squares are defended or are weak and attacked by two.
        // We exclude squares containing pieces which we cannot caputre
        uint64_t safe =  ~board->colours[THEM]
                      & (~ei->attacked[US] | (weak & ei->attackedBy2[THEM]));

        // Find square and piece combinations which would check our King
        uint64_t occupied      = board->colours[WHITE] | board->colours[BLACK];
        uint64_t knightThreats = knightAttacks(kingSq);
        uint64_t bishopThreats = bishopAttacks(kingSq, occupied);
        uint64_t rookThreats   = rookAttacks(kingSq, occupied);
        uint64_t queenThreats  = bishopThreats | rookThreats;

        // Identify if pieces can move to those checking squares safely.
        // We check if our Queen can attack the square for safe Queen checks.
        // No attacks of other pieces is implicit in our definition of weak.
        uint64_t knightChecks = knightThreats & safe &  ei->attackedBy[THEM][KNIGHT];
        uint64_t bishopChecks = bishopThreats & safe &  ei->attackedBy[THEM][BISHOP];
        uint64_t rookChecks   = rookThreats   & safe &  ei->attackedBy[THEM][ROOK  ];
        uint64_t queenChecks  = queenThreats  & safe &  ei->attackedBy[THEM][QUEEN ]
                                                     & ~ei->attackedBy[  US][QUEEN ];

        count  = ei->kingAttackersCount[THEM] * ei->kingAttackersWeight[THEM];

        count += KSAttackValue     * scaledAttackCounts
               + KSWeakSquares     * popcount(weak & ei->kingAreas[US])
               + KSFriendlyPawns   * popcount(myPawns & ei->kingAreas[US])
               + KSNoEnemyQueens   * !enemyQueens
               + KSSafeQueenCheck  * !!queenChecks
               + KSSafeRookCheck   * !!rookChecks
               + KSSafeBishopCheck * !!bishopChecks
               + KSSafeKnightCheck * !!knightChecks
               + KSAdjustment;

        // Convert safety to an MG and EG score, if we are unsafe
        if (count > 0) eval -= MakeScore(count * count / 720, count / 20);
    }

    // Shelter eval is already stored in the Pawn King Table. evaluatePawns()
    // returns the associated score, so we only need to return the usual eval
    if (ei->pkentry != NULL) return eval;

    // Evaluate King Shelter. Look at our king's file, as well as the possible two adjacent
    // files. We evaluate based on distance between our king's rang and the nearest friendly
    // pawn that is placed ahead or on rank with our king. Use a distance of 7 to denote
    // configurations which have no such pawn. 7 is not a legal distance normally.
    for (int file = MAX(0, kingFile - 1); file <= MIN(FILE_NB - 1, kingFile + 1); file++) {

        uint64_t filePawns = myPawns & Files[file] & ranksAtOrAboveMasks(US, kingRank);

        int distance = !filePawns   ? 7
                     :  US == WHITE ? rankOf(getlsb(filePawns)) - kingRank
                                    : kingRank - rankOf(getmsb(filePawns));

        pkeval += KingShelter[file == kingFile][file][distance];
        if (TRACE) T.KingShelter[file == kingFile][file][distance][US]++;
    }

    ei->pkeval[US] += pkeval;

    return eval + pkeval;
}

int evaluatePassedPawns(EvalInfo* ei, Board* board, int colour){

    int sq, rank, canAdvance, safeAdvance, eval = 0;
    uint64_t tempPawns, destination, notEmpty;

    // Fetch Passed Pawns from the Pawn King Entry if we have one
    if (ei->pkentry != NULL) ei->passedPawns = ei->pkentry->passed;

    tempPawns = board->colours[colour] & ei->passedPawns;
    notEmpty  = board->colours[WHITE ] | board->colours[BLACK];

    // Evaluate each passed pawn
    while (tempPawns != 0ull){

        // Pop off the next passed Pawn
        sq = poplsb(&tempPawns);

        // Determine the releative rank
        rank = (colour == BLACK) ? (7 - rankOf(sq)) : rankOf(sq);

        // Determine where the pawn would advance to
        destination = (colour == BLACK) ? ((1ull << sq) >> 8): ((1ull << sq) << 8);

        // Destination does not have any pieces on it
        canAdvance = !(destination & notEmpty);

        // Destination is not attacked by the opponent
        safeAdvance = !(destination & ei->attacked[!colour]);

        eval += PassedPawn[canAdvance][safeAdvance][rank];
        if (TRACE) T.PassedPawn[canAdvance][safeAdvance][rank][colour]++;
    }

    return eval;
}

int evaluateThreats(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const uint64_t Rank3Relative = US == WHITE ? RANK_3 : RANK_6;

    int count, eval = 0;

    uint64_t friendly = board->colours[  US];
    uint64_t enemy    = board->colours[THEM];
    uint64_t occupied = friendly | enemy;

    uint64_t pawns   = friendly & board->pieces[PAWN  ];
    uint64_t knights = friendly & board->pieces[KNIGHT];
    uint64_t bishops = friendly & board->pieces[BISHOP];
    uint64_t rooks   = friendly & board->pieces[ROOK  ];
    uint64_t queens  = friendly & board->pieces[QUEEN ];

    uint64_t attacksByPawns  = ei->attackedBy[THEM][PAWN  ];
    uint64_t attacksByMajors = ei->attackedBy[THEM][ROOK  ] | ei->attackedBy[THEM][QUEEN ];

    // A friendly minor / major is overloaded if attacked and defended by exactly one
    uint64_t overloaded = (knights | bishops | rooks | queens)
                        & ei->attacked[  US] & ~ei->attackedBy2[  US]
                        & ei->attacked[THEM] & ~ei->attackedBy2[THEM];

    // Pawn advances (single or double moves) which threaten an enemy piece.
    // Exclude pawn moves to squares which are weak, or attacked by enemy pawns
    uint64_t pushThreat  = pawnAdvance(pawns, occupied, US);
    pushThreat |= pawnAdvance(pushThreat & Rank3Relative, occupied, US);
    pushThreat &= ~attacksByPawns & (ei->attacked[US] | ~ei->attacked[THEM]);
    pushThreat  = pawnAttackSpan(pushThreat, enemy & ~ei->attackedBy[US][PAWN], US);

    // Penalty for each unsupported pawn on the board
    count = popcount(pawns & ~ei->attacked[US] & ei->attacked[THEM]);
    eval += count * ThreatPawnAttackedByOne;
    if (TRACE) T.ThreatPawnAttackedByOne[US] += count;

    // Penalty for pawn threats against our minors
    count = popcount((knights | bishops) & attacksByPawns);
    eval += count * ThreatMinorAttackedByPawn;
    if (TRACE) T.ThreatMinorAttackedByPawn[US] += count;

    // Penalty for all major threats against our unsupported knights and bishops
    count = popcount((knights | bishops) & ~ei->attacked[US] & attacksByMajors);
    eval += count * ThreatMinorAttackedByMajor;
    if (TRACE) T.ThreatMinorAttackedByMajor[US] += count;

    // Penalty for any threat against our queens
    count = popcount(queens & ei->attacked[THEM]);
    eval += count * ThreatQueenAttackedByOne;
    if (TRACE) T.ThreatQueenAttackedByOne[US] += count;

    // Penalty for any overloaded minors or majors
    count = popcount(overloaded);
    eval += count * ThreatOverloadedPieces;
    if (TRACE) T.ThreatOverloadedPieces[US] += count;

    // Bonus for giving threats by safe pawn pushes
    count = popcount(pushThreat);
    eval += count * ThreatByPawnPush;
    if (TRACE) T.ThreatByPawnPush[colour] += count;

    return eval;
}

void initializeEvalInfo(EvalInfo* ei, Board* board, PawnKingTable* pktable){

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];
    uint64_t pawns   = board->pieces[PAWN];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK];
    uint64_t queens  = board->pieces[QUEEN];
    uint64_t kings   = board->pieces[KING];

    uint64_t whitePawns = white & pawns;
    uint64_t blackPawns = black & pawns;

    int wKingSq = getlsb((white & kings));
    int bKingSq = getlsb((black & kings));

    ei->pawnAttacks[WHITE] = pawnAttackSpan(whitePawns, ~0ull, WHITE);
    ei->pawnAttacks[BLACK] = pawnAttackSpan(blackPawns, ~0ull, BLACK);

    ei->rammedPawns[WHITE] = pawnAdvance(blackPawns, ~whitePawns, BLACK);
    ei->rammedPawns[BLACK] = pawnAdvance(whitePawns, ~blackPawns, WHITE);

    ei->blockedPawns[WHITE] = pawnAdvance(white | black, ~whitePawns, BLACK);
    ei->blockedPawns[BLACK] = pawnAdvance(white | black, ~blackPawns, WHITE);

    ei->kingAreas[WHITE] = kingAttacks(wKingSq) | (1ull << wKingSq) | (kingAttacks(wKingSq) << 8);
    ei->kingAreas[BLACK] = kingAttacks(bKingSq) | (1ull << bKingSq) | (kingAttacks(bKingSq) >> 8);

    ei->mobilityAreas[WHITE] = ~(ei->pawnAttacks[BLACK] | (white & kings) | ei->blockedPawns[WHITE]);
    ei->mobilityAreas[BLACK] = ~(ei->pawnAttacks[WHITE] | (black & kings) | ei->blockedPawns[BLACK]);

    ei->attacked[WHITE] = ei->attackedBy[WHITE][KING] = kingAttacks(wKingSq);
    ei->attacked[BLACK] = ei->attackedBy[BLACK][KING] = kingAttacks(bKingSq);

    ei->occupiedMinusBishops[WHITE] = (white | black) ^ (white & (bishops | queens));
    ei->occupiedMinusBishops[BLACK] = (white | black) ^ (black & (bishops | queens));

    ei->occupiedMinusRooks[WHITE] = (white | black) ^ (white & (rooks | queens));
    ei->occupiedMinusRooks[BLACK] = (white | black) ^ (black & (rooks | queens));

    ei->passedPawns = 0ull;

    ei->kingAttacksCount[WHITE]    = ei->kingAttacksCount[BLACK]    = 0;
    ei->kingAttackersCount[WHITE]  = ei->kingAttackersCount[BLACK]  = 0;
    ei->kingAttackersWeight[WHITE] = ei->kingAttackersWeight[BLACK] = 0;

    if (TEXEL) ei->pkentry = NULL;
    else       ei->pkentry = getPawnKingEntry(pktable, board->pkhash);
}
