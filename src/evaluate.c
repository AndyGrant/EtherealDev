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
    const int TRACE = 1;
    const EvalTrace EmptyTrace;
    EvalTrace T;
#else
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

/* Pawn Evaluation Terms */

const int PawnIsolated = S(  -9,  -6);

const int PawnStacked = S( -25, -17);

const int PawnBackwards[2] = { S(  -8,   0), S( -26,  -3) };

const int PawnConnected32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(  -5,  -7), S(  -7,  15), S(   3,   7), S(   2,  22),
    S(   6,   3), S(  20,   8), S(  16,  13), S(   4,  18),
    S(  14,   8), S(  29,  11), S(  16,  10), S(  27,  17),
    S(  24,  23), S(  33,  38), S(  39,  34), S(  54,  40),
    S(  21,  61), S(  23,  64), S(  63,  61), S(  53,  75),
    S( 106, -12), S( 199,  17), S( 227,  22), S( 250,  76),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

/* Knight Evaluation Terms */

const int KnightOutpost[2] = { S(  14,  -8), S(  28,   0) };

const int KnightMobility[9] = {
    S( -92, -86), S( -52, -94), S( -30, -43), S( -22, -20),
    S(  -7, -17), S(   0,  -2), S(   9,   0), S(  19,  -6),
    S(  34, -15),
};

/* Bishop Evaluation Terms */

const int BishopPair = S(  26,  75);

const int BishopRammedPawns = S( -10, -17);

const int BishopOutpost[2] = { S(  22,  -8), S(  44,   9) };

const int BishopMobility[14] = {
    S( -56,-125), S( -35, -66), S( -30, -50), S( -10, -29),
    S(   2, -19), S(  10,  -1), S(  18,   7), S(  19,   8),
    S(  21,  13), S(  28,   9), S(  28,   6), S(  41, -12),
    S(  44,  -6), S(  39, -37),
};

/* Rook Evaluation Terms */

const int RookFile[2] = { S(  12,  -2), S(  33,   0) };

const int RookOnSeventh = S(  -7,  39);

const int RookMobility[15] = {
    S(-146,-107), S( -51,-118), S( -34, -62), S( -19, -28),
    S( -15, -10), S( -12,   3), S( -13,  10), S(  -9,  15),
    S(  -3,  23), S(   2,  31), S(   5,  35), S(  10,  42),
    S(  18,  42), S(  21,  35), S(  20,  45),
};


/* Queen Evaluation Terms */

const int QueenMobility[28] = {
    S( -61,-263), S(-217,-390), S( -48,-205), S( -40,-189),
    S( -26,-131), S( -22, -68), S( -13, -90), S( -14, -74),
    S( -11, -62), S(  -8, -51), S(  -6, -33), S(  -2, -24),
    S(   0, -18), S(   2, -11), S(   2,  -2), S(   2,   5),
    S(   0,  14), S(   0,  12), S(   4,  22), S(  -4,  18),
    S(  -4,  17), S(  17,  20), S(   4,  -2), S(  30,   3),
    S(  34,  11), S(  57,  -7), S( -51, -19), S(   0,  -2),
};

/* King Evaluation Terms */

const int KingDefenders[12] = {
    S( -27,  -4), S( -15,   4), S(  -2,   0), S(   7,   4),
    S(  17,   0), S(  32,  -1), S(  39,  11), S(  23,   0),
    S(  12,   6), S(  12,   6), S(  12,   6), S(  12,   6),
};

const int KingShelter[2][8][8] = {
  {{S( -17,  13), S(  22,  -6), S(  13,   0), S(  23,   0), S(   6,   5), S(  29,   3), S(   0, -32), S( -30,   8)},
   {S(   0,   1), S(  20, -12), S(  21,  -6), S(  -4, -12), S( -26,   2), S( -66,  78), S( 101,  94), S( -27,   4)},
   {S(  17,   4), S(  12,  -7), S( -15,   2), S(  -8,   0), S( -20,   0), S(  14,  -8), S(   4,  65), S( -20,   8)},
   {S(  12,  29), S(   3,  -2), S(  -7, -10), S(  20,  -9), S(  22, -35), S( -20, -31), S(-147,  40), S(  -4,   3)},
   {S( -17,  18), S(  -7,  -4), S( -24,   3), S(  -7,   2), S( -17, -11), S( -40, -19), S(  38,  -7), S( -10,   0)},
   {S(  19,  -4), S(  13,   1), S( -13,   9), S( -11, -22), S(   2, -37), S(  14, -49), S(  50, -33), S( -26,   2)},
   {S(  19,  -4), S(   9, -13), S( -35,  -2), S( -16, -14), S( -24, -19), S( -44,   8), S(   7,  47), S( -18,   9)},
   {S( -20,  -4), S(   2,  -1), S(   8,   1), S(  -1,   3), S( -20,   7), S(   1,  34), S(-200,  79), S( -28,   8)}},
  {{S(   0,   0), S(   8, -11), S(   9, -13), S( -46,  18), S(   0, -10), S(   9,  51), S(-170, -14), S( -49,   8)},
   {S(   0,   0), S(  23,  -9), S(  10,  -6), S(  -5,  -5), S(  11, -31), S(  51, 102), S(-188, -11), S( -42,   7)},
   {S(   0,   0), S(  30,  -9), S(  -2, -10), S(  20, -24), S(  15,  -7), S(-114,  44), S( -89, -83), S( -24,   6)},
   {S(   0,   0), S(  -4,   3), S(  -7,  13), S( -13,   1), S( -23,   0), S(-116,  10), S(   4, -50), S( -35,   1)},
   {S(   0,   0), S( -14,   2), S( -11,   0), S(  28,  -3), S(  36, -21), S( -44,   7), S(-108, -72), S(  -5,  -2)},
   {S(   0,   0), S(  20,  -4), S(  -5,   2), S(  -5, -11), S(  29, -38), S( -45,   4), S(  58,  46), S( -37,   1)},
   {S(   0,   0), S(   9,  -5), S( -13,   2), S( -17,  -6), S( -27, -15), S(  20,  10), S( -57, -63), S( -42,  17)},
   {S(   0,   0), S(   7, -33), S(  15, -14), S( -28,  -1), S( -26,  -1), S(   6, -16), S(-239, -73), S( -54,  14)}},
};

/* King Safety Evaluation Terms */

const int KSAttackWeight[]  = { 0, 16, 6, 10, 8, 0 };
const int KSAttackValue     =   44;
const int KSWeakSquares     =   38;
const int KSFriendlyPawns   =  -22;
const int KSNoEnemyQueens   = -276;
const int KSSafeQueenCheck  =   95;
const int KSSafeRookCheck   =   94;
const int KSSafeBishopCheck =   51;
const int KSSafeKnightCheck =  123;
const int KSAdjustment      =  -18;

/* Passed Pawn Evaluation Terms */

const int PassedPawn[2][2][8] = {
  {{S(   0,   0), S( -29, -25), S( -27,   4), S( -10,   0), S(  31,  12), S(  65,   6), S( 154,  41), S(   0,   0)},
   {S(   0,   0), S(   0,   2), S( -19,  14), S( -12,  32), S(  19,  52), S(  64,  74), S( 192, 141), S(   0,   0)}},
  {{S(   0,   0), S(   3,  13), S(  -2,   7), S(  -5,  28), S(  34,  50), S(  82,  89), S( 239, 165), S(   0,   0)},
   {S(   0,   0), S(   0,   4), S(  -4,   9), S( -10,  52), S(   6, 108), S(  42, 209), S( 126, 374), S(   0,   0)}},
};

/* Threat Evaluation Terms */

const int ThreatWeakPawn             = S( -10, -36);
const int ThreatMinorAttackedByPawn  = S( -54, -51);
const int ThreatMinorAttackedByMajor = S( -42, -42);
const int ThreatRookAttackedByLesser = S( -48, -31);
const int ThreatQueenAttackedByOne   = S( -58,   0);
const int ThreatOverloadedPieces     = S(  -9, -11);
const int ThreatByPawnPush           = S(  11,  22);


/* General Evaluation Terms */

const int Tempo[COLOUR_NB] = { S(  25,  12), S( -25, -12) };

#undef S

int evaluateBoard(Board* board, PawnKingTable* pktable){

    EvalInfo ei;
    int phase, factor, eval, pkeval;

    // Check for guaranteed material draws
    if (evaluateDraws(board)) return 0;

    // Setup and perform all evaluations
    initializeEvalInfo(&ei, board, pktable);
    eval   = evaluatePieces(&ei, board);
    pkeval = ei.pkeval[WHITE] - ei.pkeval[BLACK];
    eval  += pkeval + board->psqtmat + Tempo[board->turn];

    // Calcuate the game phase based on remaining material (Fruit Method)
    phase = 24 - 4 * popcount(board->pieces[QUEEN ])
               - 2 * popcount(board->pieces[ROOK  ])
               - 1 * popcount(board->pieces[KNIGHT]
                             |board->pieces[BISHOP]);
    phase = (phase * 256 + 12) / 24;

    // Scale evaluation based on remaining material
    factor = evaluateScaleFactor(board);

    // Compute the interpolated and scaled evaluation
    eval = (ScoreMG(eval) * (256 - phase)
         +  ScoreEG(eval) * phase * factor / SCALE_NORMAL) / 256;

    // Store a new Pawn King Entry if we did not have one
    if (ei.pkentry == NULL)
        storePawnKingEntry(pktable, board->pkhash, ei.passedPawns, pkeval);

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

    int sq, semi, eval = 0, pkeval = 0;
    uint64_t pawns, myPawns, tempPawns, enemyPawns, attacks;

    // Store off pawn attacks for king safety and threat computations
    ei->attackedBy2[US]      = ei->pawnAttacks[US] & ei->attacked[US];
    ei->attacked[US]        |= ei->pawnAttacks[US];
    ei->attackedBy[US][PAWN] = ei->pawnAttacks[US];

    // Update attacker counts for King Safety computation
    attacks = ei->pawnAttacks[US] & ei->kingAreas[THEM];
    ei->kingAttacksCount[US] += popcount(attacks);

    // Pawn hash holds the rest of the pawn evaluation
    if (ei->pkentry != NULL) return eval;

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
        if (!(isolatedPawnMasks(sq) & myPawns)) {
            pkeval += PawnIsolated;
            if (TRACE) T.PawnIsolated[US]++;
        }

        // Apply a penalty if the pawn is stacked
        if (Files[fileOf(sq)] & tempPawns) {
            pkeval += PawnStacked;
            if (TRACE) T.PawnStacked[US]++;
        }

        // Apply a penalty if the pawn is backward
        if (   !(passedPawnMasks(THEM, sq) & myPawns)
            &&  (testBit(ei->pawnAttacks[THEM], sq + Forward))) {
            semi = !(Files[fileOf(sq)] & enemyPawns);
            pkeval += PawnBackwards[semi];
            if (TRACE) T.PawnBackwards[semi][US]++;
        }

        // Apply a bonus if the pawn is connected and not backward
        else if (pawnConnectedMasks(US, sq) & myPawns) {
            pkeval += PawnConnected32[relativeSquare32(sq, US)];
            if (TRACE) T.PawnConnected32[relativeSquare32(sq, US)][US]++;
        }
    }

    ei->pkeval[US] = pkeval; // Save eval for the Pawn Hash

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
        // We exclude squares containing pieces which we cannot capture.
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
               + KSFriendlyPawns   * popcount(myPawns & ei->kingAreas[US] & ~weak)
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

    return eval;
}

int evaluatePassedPawns(EvalInfo* ei, Board* board, int colour){

    int sq, rank, canAdvance, safeAdvance, eval = 0;
    uint64_t destination;

    uint64_t tempPawns = board->colours[colour] & ei->passedPawns;
    uint64_t notEmpty  = board->colours[WHITE ] | board->colours[BLACK];

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
    uint64_t attacksByMinors = ei->attackedBy[THEM][KNIGHT] | ei->attackedBy[THEM][BISHOP];
    uint64_t attacksByMajors = ei->attackedBy[THEM][ROOK  ] | ei->attackedBy[THEM][QUEEN ];

    // Squares with more attackers, few defenders, and no pawn support
    uint64_t poorlyDefended = (ei->attacked[THEM] & ~ei->attacked[US])
                            | (ei->attackedBy2[THEM] & ~ei->attackedBy2[US] & ~ei->attackedBy[US][PAWN]);

    // A friendly minor / major is overloaded if attacked and defended by exactly one
    uint64_t overloaded = (knights | bishops | rooks | queens)
                        & ei->attacked[  US] & ~ei->attackedBy2[  US]
                        & ei->attacked[THEM] & ~ei->attackedBy2[THEM];

    // Pawn advances by a single square which threaten an enemy piece.
    // Exclude pawn moves to squares which are weak, or attacked by enemy pawns
    uint64_t pushThreat  = pawnAdvance(pawns, occupied, US);
    pushThreat &= ~attacksByPawns & (ei->attacked[US] | ~ei->attacked[THEM]);
    pushThreat  = pawnAttackSpan(pushThreat, enemy & ~ei->attackedBy[US][PAWN], US);

    // Penalty for each of our poorly supported pawns
    count = popcount(pawns & ~attacksByPawns & poorlyDefended);
    eval += count * ThreatWeakPawn;
    if (TRACE) T.ThreatWeakPawn[US] += count;

    // Penalty for pawn threats against our minors
    count = popcount((knights | bishops) & attacksByPawns);
    eval += count * ThreatMinorAttackedByPawn;
    if (TRACE) T.ThreatMinorAttackedByPawn[US] += count;

    // Penalty for all major threats against our unsupported knights and bishops
    count = popcount((knights | bishops) & poorlyDefended & attacksByMajors);
    eval += count * ThreatMinorAttackedByMajor;
    if (TRACE) T.ThreatMinorAttackedByMajor[US] += count;

    // Penalty for pawn and minor threats against our rooks
    count = popcount(rooks & (attacksByPawns | attacksByMinors));
    eval += count * ThreatRookAttackedByLesser;
    if (TRACE) T.ThreatRookAttackedByLesser[US] += count;

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

int evaluateScaleFactor(Board *board) {

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];
    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    if (    onlyOne(white & bishops)
        &&  onlyOne(black & bishops)
        &&  onlyOne(bishops & WHITE_SQUARES)) {

        if (!(knights | rooks | queens))
            return SCALE_OCB_BISHOPS_ONLY;

        if (   !(rooks | queens)
            &&  onlyOne(white & knights)
            &&  onlyOne(black & knights))
            return SCALE_OCB_ONE_KNIGHT;

        if (   !(knights | queens)
            && onlyOne(white & rooks)
            && onlyOne(black & rooks))
            return SCALE_OCB_ONE_ROOK;

        if (   !(knights | queens)
            && several(white & rooks)
            && several(black & rooks))
            return SCALE_OCB_TWO_ROOKS;

        return SCALE_OCB_GENERAL;
    }

    return SCALE_NORMAL;
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

    ei->kingAttacksCount[WHITE]    = ei->kingAttacksCount[BLACK]    = 0;
    ei->kingAttackersCount[WHITE]  = ei->kingAttackersCount[BLACK]  = 0;
    ei->kingAttackersWeight[WHITE] = ei->kingAttackersWeight[BLACK] = 0;

    ei->pkentry       = TRACE ? NULL : getPawnKingEntry(pktable, board->pkhash);
    ei->passedPawns   = ei->pkentry == NULL ? 0ull : ei->pkentry->passed;
    ei->pkeval[WHITE] = ei->pkentry == NULL ? 0    : ei->pkentry->eval;
    ei->pkeval[BLACK] = ei->pkentry == NULL ? 0    : 0;
}
