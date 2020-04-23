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

#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "masks.h"
#include "transposition.h"
#include "types.h"

EvalTrace T, EmptyTrace;
int PSQT[32][SQUARE_NB];

#define S(mg, eg) (MakeScore((mg), (eg)))

/* Material Value Evaluation Terms */

const int PawnValue   = S( 107, 127);
const int KnightValue = S( 454, 414);
const int BishopValue = S( 476, 431);
const int RookValue   = S( 658, 690);
const int QueenValue  = S(1289,1372);
const int KingValue   = S(   0,   0);

/* Piece Square Evaluation Terms */

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S( -20,   5), S(   3,   0), S( -12,   3), S(  -8,  -3),
    S( -25,   2), S( -15,   2), S( -10,  -7), S(  -4, -15),
    S( -19,  10), S( -14,  11), S(   8, -12), S(   9, -25),
    S(  -9,  12), S(  -4,   8), S(  -7,  -7), S(  -1, -22),
    S(  -6,  25), S(  -4,  22), S(   2,  11), S(  30, -16),
    S( -18, -48), S( -67, -17), S(   1, -31), S(  38, -45),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -49, -26), S(  -8, -32), S( -16, -29), S(  -1, -17),
    S(  -3, -21), S(   3, -12), S(   1, -29), S(   9, -19),
    S(   9, -24), S(  22, -22), S(  14, -19), S(  24,  -5),
    S(  22,   4), S(  24,   6), S(  32,  15), S(  31,  21),
    S(  28,  17), S(  30,   9), S(  42,  25), S(  34,  35),
    S( -14,  15), S(   6,  12), S(  33,  26), S(  31,  28),
    S(  10, -10), S(  -7,   1), S(  32, -19), S(  43,   1),
    S(-171, -19), S( -81,  -2), S(-110,  19), S( -30,   0),
};

const int BishopPSQT32[32] = {
    S(   4, -26), S(  12, -17), S( -11,  -7), S(   9, -10),
    S(  32, -32), S(   9, -36), S(  22, -18), S(  13, -10),
    S(  16, -10), S(  30, -13), S(   3, -17), S(  24,  -3),
    S(  17,  -7), S(  21,  -2), S(  18,   5), S(  27,   7),
    S( -10,  12), S(  22,   5), S(   7,  14), S(  15,  18),
    S(   2,   6), S(  -2,  16), S(  -1,   3), S(  19,  13),
    S( -46,  15), S( -57,   2), S(  -8,   8), S( -21,  13),
    S( -52,  -7), S( -47,   9), S( -87,  16), S( -90,  28),
};

const int RookPSQT32[32] = {
    S( -10, -30), S( -14, -20), S(  -1, -24), S(   7, -28),
    S( -56, -15), S( -12, -33), S(  -8, -32), S(   4, -35),
    S( -29, -16), S(  -5, -12), S( -17, -16), S(   0, -24),
    S( -16,  -2), S(  -9,   7), S(  -7,   3), S(   6,  -3),
    S(  -2,  11), S(  14,   9), S(  24,   5), S(  41,   1),
    S( -12,  22), S(  29,   9), S(   4,  22), S(  37,   5),
    S(   1,   7), S( -17,  16), S(   8,   6), S(  23,   8),
    S(  37,  21), S(  26,  24), S(   7,  30), S(  17,  26),
};

const int QueenPSQT32[32] = {
    S(  25, -53), S(   4, -39), S(  11, -51), S(  20, -42),
    S(  14, -39), S(  28, -55), S(  31, -69), S(  19, -28),
    S(  11, -23), S(  26, -20), S(   7,   4), S(   7,  -2),
    S(  12,  -4), S(  16,  15), S(  -2,  20), S( -17,  61),
    S(  -5,  17), S(  -6,  42), S( -13,  21), S( -32,  70),
    S( -23,  28), S( -15,  19), S( -23,  23), S( -12,  25),
    S(  -4,  27), S( -63,  63), S(  -8,  15), S( -42,  53),
    S(  -9,  16), S(  18,   9), S(   9,   9), S(  -6,  20),
};

const int KingPSQT32[32] = {
    S(  48, -82), S(  45, -54), S( -11, -15), S( -26, -24),
    S(  30, -28), S(  -8, -18), S( -41,   7), S( -57,   9),
    S(   5, -29), S(  14, -25), S(  16,  -2), S( -12,  14),
    S(   3, -33), S(  86, -34), S(  43,   6), S(  -5,  25),
    S(   6, -15), S(  97, -29), S(  48,  13), S(   4,  22),
    S(  46, -25), S( 122, -20), S(  95,   6), S(  39,   3),
    S(   5, -47), S(  47,  -7), S(  32,   8), S(   9,   0),
    S(   8,-102), S(  74, -54), S( -20, -14), S( -18, -12),
};

/* Pawn Evaluation Terms */

const int PawnCandidatePasser[2][RANK_NB] = {
   {S(   0,   0), S( -27, -10), S( -12,   9), S( -17,  29),
    S(  -1,  59), S(  42,  66), S(   0,   0), S(   0,   0)},
   {S(   0,   0), S( -14,  18), S(  -5,  21), S(   2,  45),
    S(  17,  81), S(  32,  55), S(   0,   0), S(   0,   0)},
};

const int PawnIsolated = S(  -7, -12);

const int PawnStacked[2] = { S(  -5, -21), S(  -8,  -9) };

const int PawnBackwards[2][RANK_NB] = {
   {S(   0,   0), S(   5,  -5), S(   9,  -5), S(  10, -12),
    S(  12, -10), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(   0,   0), S( -11, -26), S(  -6, -21), S(   5, -23),
    S(  20, -18), S(   0,   0), S(   0,   0), S(   0,   0)},
};

const int PawnConnected32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(  -2,  -6), S(  13,   0), S(   3,   1), S(   6,  12),
    S(  16,   0), S(  30,  -4), S(  22,   6), S(  26,  13),
    S(  10,  -1), S(  26,   2), S(  10,   9), S(  18,  16),
    S(  11,  10), S(  23,  14), S(  29,  21), S(  31,  20),
    S(  58,  23), S(  49,  46), S(  69,  55), S(  84,  58),
    S( 112,   0), S( 204,   9), S( 228,  30), S( 240,  49),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

/* Knight Evaluation Terms */

const int KnightOutpost[2][2] = {
   {S(  10, -24), S(  37,  -2)},
   {S(   4, -22), S(  20,  -5)},
};

const int KnightBehindPawn = S(   4,  18);

const int KnightInSiberia[4] = {
    S( -10,   0), S( -12,  -5), S( -23,  -5), S( -20,  -3),
};

const int KnightMobility[9] = {
    S( -75,-104), S( -31, -97), S( -16, -40), S(  -2, -15),
    S(   9,  -5), S(  14,  11), S(  22,  15), S(  33,  15),
    S(  46,   1),
};

/* Bishop Evaluation Terms */

const int BishopPair = S(  22,  69);

const int BishopRammedPawns = S(  -7, -14);

const int BishopOutpost[2][2] = {
   {S(  13, -10), S(  46,   1)},
   {S(   7, -10), S(  13,  -4)},
};

const int BishopBehindPawn = S(   3,  18);

const int BishopLongDiagonal = S(  24,  13);

const int BishopMobility[14] = {
    S( -67,-147), S( -31, -93), S( -10, -50), S(   1, -25),
    S(  11, -14), S(  18,  -1), S(  21,   9), S(  22,  14),
    S(  21,  20), S(  25,  21), S(  25,  21), S(  45,  10),
    S(  43,  19), S(  71, -15),
};

/* Rook Evaluation Terms */

const int RookFile[2] = { S(  15,   4), S(  36,   2) };

const int RookOnSeventh = S(  -3,  31);

const int RookMobility[15] = {
    S(-149,-113), S( -58,-113), S( -15, -57), S(  -6, -19),
    S(  -6,   0), S(  -7,  15), S(  -6,  26), S(   0,  29),
    S(   8,  32), S(  11,  37), S(  14,  43), S(  20,  47),
    S(  20,  52), S(  35,  41), S(  89,   1),
};

/* Queen Evaluation Terms */

const int QueenMobility[28] = {
    S( -61,-263), S(-210,-387), S( -58,-201), S( -21,-192),
    S(  -6,-139), S(  -3, -82), S(   2, -50), S(   2, -20),
    S(   7, -16), S(   8,   5), S(  13,  10), S(  14,  26),
    S(  16,  20), S(  18,  29), S(  15,  34), S(  11,  41),
    S(  11,  41), S(   2,  44), S(   4,  41), S(  -1,  36),
    S(   8,  13), S(  22,  -9), S(  26, -38), S(  28, -55),
    S(  11, -70), S(  23, -97), S( -56, -39), S( -31, -62),
};

/* King Evaluation Terms */

const int KingDefenders[12] = {
    S( -22,  -1), S(  -6,  -2), S(   1,   3), S(   8,   6),
    S(  15,   6), S(  26,   2), S(  29,  -3), S(  13,   0),
    S(  12,   6), S(  12,   6), S(  12,   6), S(  12,   6),
};

const int KingPawnFileProximity[FILE_NB] = {
    S(  35,  36), S(  17,  27), S(   5,  14), S( -17, -16),
    S( -19, -52), S( -17, -68), S( -16, -71), S( -11, -70),
};

const int KingShelter[2][FILE_NB][RANK_NB] = {
  {{S( -12,   2), S(  15, -25), S(  25,  -9), S(  15,   5),
    S(   8,   4), S(   0,   2), S(  -3, -33), S( -51,  18)},
   {S(  17,  -7), S(  23, -17), S(   2,  -6), S( -14,   3),
    S( -30,  15), S( -71,  68), S(  92,  82), S( -22,   2)},
   {S(  36,  -3), S(  14,  -9), S( -30,   5), S( -13,  -9),
    S( -20,  -4), S( -12,   1), S(   0,  66), S( -14,  -1)},
   {S(   3,  10), S(  25, -13), S(   5, -12), S(  14, -20),
    S(  29, -36), S( -59,   5), S(-136,  52), S(   7,  -6)},
   {S( -17,   9), S(   3,  -4), S( -31,   1), S( -22,   5),
    S( -22,  -8), S( -42,  -2), S(  33, -17), S(  -9,   0)},
   {S(  49, -16), S(  23, -15), S( -22,   2), S( -13, -19),
    S(   5, -25), S(  17, -20), S(  41, -30), S( -26,   1)},
   {S(  29, -14), S(   0, -16), S( -25,  -2), S( -22,  -8),
    S( -31,  -8), S( -37,  31), S(   0,  44), S( -10,   1)},
   {S( -10, -12), S(   6, -18), S(  10,   0), S(  -2,  10),
    S( -12,  15), S( -10,  37), S(-190,  87), S( -20,  15)}},
  {{S(   0,   0), S( -12, -24), S(   6, -20), S( -40,  16),
    S( -23,   2), S(   3,  42), S(-167,  -8), S( -44,  12)},
   {S(   0,   0), S(  27, -21), S(   8,  -7), S( -20,  -2),
    S(  -1, -13), S(  26,  65), S(-184,  -3), S( -35,   6)},
   {S(   0,   0), S(  30, -12), S(  -1, -11), S(   6, -19),
    S(  16,  -6), S( -87,  47), S( -84, -73), S( -18,  -2)},
   {S(   0,   0), S(  -7,   6), S(  -6,  -3), S( -22,   1),
    S( -28,   1), S( -99,  31), S(   7, -41), S( -20,  -1)},
   {S(   0,   0), S(  13,   2), S(  12,  -7), S(  14, -12),
    S(  14, -27), S( -57,  15), S(-104, -61), S(   0,  -1)},
   {S(   0,   0), S(   3,  -6), S( -26,   1), S( -28,  -8),
    S(  18, -24), S( -38,   3), S(  55,  38), S( -16,  -4)},
   {S(   0,   0), S(  25, -15), S(  14, -14), S( -11,  -9),
    S( -31,   9), S(  -9,  16), S( -56, -49), S( -33,  14)},
   {S(   0,   0), S(  15, -40), S(  24, -31), S( -19,  -7),
    S( -18,  17), S(  -6,  20), S(-228, -55), S( -22,   5)}},
};

const int KingStorm[2][FILE_NB/2][RANK_NB] = {
  {{S(  -5,  29), S( 116,  -6), S( -25,  23), S( -16,   4),
    S( -13,   1), S(  -7,  -4), S( -17,   5), S( -25,   3)},
   {S(  -4,  51), S(  56,  14), S( -19,  20), S(  -3,   7),
    S(  -2,   3), S(   6,  -5), S(  -2,   2), S( -14,   5)},
   {S(   7,  41), S(  14,  26), S( -24,  18), S( -12,   6),
    S(   2,   1), S(   5,   0), S(  10,  -5), S(   2,   4)},
   {S(  -2,  27), S(  12,  21), S( -21,   7), S( -18,  -1),
    S( -15,   1), S(   7, -11), S(   1,  -8), S( -14,   5)}},
  {{S(   0,   0), S( -16, -16), S( -18,  -3), S(  24, -18),
    S(  12,  -5), S(   5, -17), S(  -2,   0), S(  17,  29)},
   {S(   0,   0), S( -16, -35), S(  -1, -11), S(  41, -12),
    S(   0,  -1), S(  14, -22), S(  -7, -10), S( -18,   1)},
   {S(   0,   0), S( -29, -50), S( -27,  -9), S(  13, -12),
    S(   3,  -1), S(  -9, -13), S( -12, -14), S( -11,   6)},
   {S(   0,   0), S(  -2, -18), S( -18, -18), S( -16,  -3),
    S(  -4,  -6), S(   5, -25), S(  73, -11), S(  13,  20)}},
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

const int PassedPawn[2][2][RANK_NB] = {
  {{S(   0,   0), S( -38,   2), S( -50,  23), S( -77,  29),
    S(  -2,  13), S(  71,  -6), S( 157,  55), S(   0,   0)},
   {S(   0,   0), S( -30,   9), S( -49,  31), S( -72,  37),
    S( -10,  36), S(  94,  28), S( 184,  95), S(   0,   0)}},
  {{S(   0,   0), S( -24,  18), S( -48,  28), S( -72,  42),
    S(   0,  40), S(  90,  38), S( 262, 109), S(   0,   0)},
   {S(   0,   0), S( -31,  16), S( -44,  27), S( -67,  48),
    S(   3,  58), S(  97, 114), S( 160, 263), S(   0,   0)}},
};

const int PassedFriendlyDistance[FILE_NB] = {
    S(   0,   0), S(   1,   2), S(   4,  -4), S(   7, -10),
    S(   6, -14), S(  -8, -11), S( -16,  -6), S(   0,   0),
};

const int PassedEnemyDistance[FILE_NB] = {
    S(   0,   0), S(   3,  -1), S(   4,   0), S(   9,   8),
    S(   2,  21), S(   7,  29), S(  28,  24), S(   0,   0),
};

const int PassedSafePromotionPath    = S( -24,  39);

/* Threat Evaluation Terms */

const int ThreatWeakPawn             = S( -11, -29);
const int ThreatMinorAttackedByPawn  = S( -52, -53);
const int ThreatMinorAttackedByMinor = S( -24, -36);
const int ThreatMinorAttackedByMajor = S( -21, -47);
const int ThreatRookAttackedByLesser = S( -48, -19);
const int ThreatMinorAttackedByKing  = S( -24, -21);
const int ThreatRookAttackedByKing   = S( -18, -24);
const int ThreatQueenAttackedByOne   = S( -37, -35);
const int ThreatOverloadedPieces     = S(  -7, -14);
const int ThreatByPawnPush           = S(  14,  23);

/* Space Evaluation Terms */

const int SpaceRestrictPiece = S(  -4,   0);
const int SpaceRestrictEmpty = S(  -5,  -2);
const int SpaceCenterControl = S(   2,  -3);

/* Closedness Evaluation Terms */

const int ClosednessKnightAdjustment[9] = {
    S(  -8,  -5), S(  -5,  10), S(  -5,  17), S(   0,  17),
    S(   3,  20), S(   5,  15), S(   9,  11), S(  -5,  27),
    S(  -5,  16),
};

const int ClosednessRookAdjustment[9] = {
    S(  51,   4), S(  10,  33), S(   7,  19), S(   0,   5),
    S(  -3,   5), S(  -5,  -7), S( -10, -10), S( -21, -15),
    S( -28, -17),
};

/* Complexity Evaluation Terms */

const int ComplexityTotalPawns  = S(   0,   9);
const int ComplexityPawnFlanks  = S(   0,  80);
const int ComplexityPawnEndgame = S(   0,  48);
const int ComplexityAdjustment  = S(   0,-142);

/* General Evaluation Terms */

const int Tempo = 20;

#undef S

int evaluateBoard(Board *board, PKTable *pktable, int contempt) {

    EvalInfo ei;
    int phase, factor, eval, pkeval;

    // Setup and perform all evaluations
    initEvalInfo(&ei, board, pktable);
    eval   = evaluatePieces(&ei, board);
    pkeval = ei.pkeval[WHITE] - ei.pkeval[BLACK];
    eval  += pkeval + board->psqtmat;
    eval  += contempt;
    eval  += evaluateClosedness(&ei, board);
    eval  += evaluateComplexity(&ei, board, eval);

    // Calculate the game phase based on remaining material (Fruit Method)
    phase = 24 - 4 * popcount(board->pieces[QUEEN ])
               - 2 * popcount(board->pieces[ROOK  ])
               - 1 * popcount(board->pieces[KNIGHT]
                             |board->pieces[BISHOP]);
    phase = (phase * 256 + 12) / 24;

    // Scale evaluation based on remaining material
    factor = evaluateScaleFactor(board, eval);

    // Compute the interpolated and scaled evaluation
    eval = (ScoreMG(eval) * (256 - phase)
         +  ScoreEG(eval) * phase * factor / SCALE_NORMAL) / 256;

    // Factor in the Tempo after interpolation and scaling, so that
    // in the search we can assume that if a null move is made, then
    // then `eval = last_eval + 2 * Tempo`
    eval += board->turn == WHITE ? Tempo : -Tempo;

    // Store a new Pawn King Entry if we did not have one
    if (ei.pkentry == NULL && pktable != NULL)
        storePKEntry(pktable, board->pkhash, ei.passedPawns, pkeval);

    // Return the evaluation relative to the side to move
    return board->turn == WHITE ? eval : -eval;
}

int evaluatePieces(EvalInfo *ei, Board *board) {

    int eval;

    eval  =   evaluatePawns(ei, board, WHITE)   - evaluatePawns(ei, board, BLACK);
    eval += evaluateKnights(ei, board, WHITE) - evaluateKnights(ei, board, BLACK);
    eval += evaluateBishops(ei, board, WHITE) - evaluateBishops(ei, board, BLACK);
    eval +=   evaluateRooks(ei, board, WHITE)   - evaluateRooks(ei, board, BLACK);
    eval +=  evaluateQueens(ei, board, WHITE)  - evaluateQueens(ei, board, BLACK);
    eval +=   evaluateKings(ei, board, WHITE)   - evaluateKings(ei, board, BLACK);
    eval +=  evaluatePassed(ei, board, WHITE)  - evaluatePassed(ei, board, BLACK);
    eval += evaluateThreats(ei, board, WHITE) - evaluateThreats(ei, board, BLACK);
    eval +=   evaluateSpace(ei, board, WHITE) -   evaluateSpace(ei, board, BLACK);

    return eval;
}

int evaluatePawns(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const int Forward = (colour == WHITE) ? 8 : -8;

    int sq, flag, eval = 0, pkeval = 0;
    uint64_t pawns, myPawns, tempPawns, enemyPawns, attacks;

    // Store off pawn attacks for king safety and threat computations
    ei->attackedBy2[US]      = ei->pawnAttacks[US] & ei->attacked[US];
    ei->attacked[US]        |= ei->pawnAttacks[US];
    ei->attackedBy[US][PAWN] = ei->pawnAttacks[US];

    // Update King Safety calculations
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
        if (TRACE) T.PawnPSQT32[relativeSquare32(US, sq)][US]++;

        uint64_t neighbors   = myPawns    & adjacentFilesMasks(fileOf(sq));
        uint64_t backup      = myPawns    & passedPawnMasks(THEM, sq);
        uint64_t stoppers    = enemyPawns & passedPawnMasks(US, sq);
        uint64_t threats     = enemyPawns & pawnAttacks(US, sq);
        uint64_t support     = myPawns    & pawnAttacks(THEM, sq);
        uint64_t pushThreats = enemyPawns & pawnAttacks(US, sq + Forward);
        uint64_t pushSupport = myPawns    & pawnAttacks(THEM, sq + Forward);
        uint64_t leftovers   = stoppers ^ threats ^ pushThreats;

        // Save passed pawn information for later evaluation
        if (!stoppers) setBit(&ei->passedPawns, sq);

        // Apply a bonus for pawns which will become passers by advancing a
        // square then exchanging our supporters with the remaining stoppers
        else if (!leftovers && popcount(pushSupport) >= popcount(pushThreats)) {
            flag = popcount(support) >= popcount(threats);
            pkeval += PawnCandidatePasser[flag][relativeRankOf(US, sq)];
            if (TRACE) T.PawnCandidatePasser[flag][relativeRankOf(US, sq)][US]++;
        }

        // Apply a penalty if the pawn is isolated. We consider pawns that
        // are able to capture another pawn to not be isolated, as they may
        // have the potential to deisolate by capturing, or be traded away
        if (!threats && !neighbors) {
            pkeval += PawnIsolated;
            if (TRACE) T.PawnIsolated[US]++;
        }

        // Apply a penalty if the pawn is stacked. We adjust the bonus for when
        // the pawn appears to be a candidate to unstack. This occurs when the
        // pawn is not passed but may capture or be recaptured by our own pawns,
        // and when the pawn may freely advance on a file and then be traded away
        if (several(Files[fileOf(sq)] & myPawns)) {
            flag = (stoppers && (threats || neighbors))
                || (stoppers & ~forwardFileMasks(US, sq));
            pkeval += PawnStacked[flag];
            if (TRACE) T.PawnStacked[flag][US]++;
        }

        // Apply a penalty if the pawn is backward. We follow the usual definition
        // of backwards, but also specify that the pawn is not both isolated and
        // backwards at the same time. We don't give backward pawns a connected bonus
        if (neighbors && pushThreats && !backup) {
            flag = !(Files[fileOf(sq)] & enemyPawns);
            pkeval += PawnBackwards[flag][relativeRankOf(US, sq)];
            if (TRACE) T.PawnBackwards[flag][relativeRankOf(US, sq)][US]++;
        }

        // Apply a bonus if the pawn is connected and not backwards. We consider a
        // pawn to be connected when there is a pawn lever or the pawn is supported
        else if (pawnConnectedMasks(US, sq) & myPawns) {
            pkeval += PawnConnected32[relativeSquare32(US, sq)];
            if (TRACE) T.PawnConnected32[relativeSquare32(US, sq)][US]++;
        }
    }

    ei->pkeval[US] = pkeval; // Save eval for the Pawn Hash

    return eval;
}

int evaluateKnights(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, outside, kingDistance, defended, count, eval = 0;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempKnights = board->pieces[KNIGHT] & board->colours[US  ];

    ei->attackedBy[US][KNIGHT] = 0ull;

    // Evaluate each knight
    while (tempKnights) {

        // Pop off the next knight
        sq = poplsb(&tempKnights);
        if (TRACE) T.KnightValue[US]++;
        if (TRACE) T.KnightPSQT32[relativeSquare32(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = knightAttacks(sq);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][KNIGHT] |= attacks;

        // Apply a bonus if the knight is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the knight
        if (     testBit(outpostRanksMasks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            outside  = testBit(FILE_A | FILE_H, sq);
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += KnightOutpost[outside][defended];
            if (TRACE) T.KnightOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the knight is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += KnightBehindPawn;
            if (TRACE) T.KnightBehindPawn[US]++;
        }

        // Apply a penalty if the knight is far from both kings
        kingDistance = MIN(distanceBetween(sq, ei->kingSquare[THEM]), distanceBetween(sq, ei->kingSquare[US]));
        if (kingDistance >= 4) {
            eval += KnightInSiberia[kingDistance - 4];
            if (TRACE) T.KnightInSiberia[kingDistance - 4][US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += KnightMobility[count];
        if (TRACE) T.KnightMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[KNIGHT];
        }
    }

    return eval;
}

int evaluateBishops(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, outside, defended, count, eval = 0;
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
        if (TRACE) T.BishopPSQT32[relativeSquare32(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = bishopAttacks(sq, ei->occupiedMinusBishops[US]);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][BISHOP] |= attacks;

        // Apply a penalty for the bishop based on number of rammed pawns
        // of our own colour, which reside on the same shade of square as the bishop
        count = popcount(ei->rammedPawns[US] & squaresOfMatchingColour(sq));
        eval += count * BishopRammedPawns;
        if (TRACE) T.BishopRammedPawns[US] += count;

        // Apply a bonus if the bishop is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the bishop.
        if (     testBit(outpostRanksMasks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            outside  = testBit(FILE_A | FILE_H, sq);
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += BishopOutpost[outside][defended];
            if (TRACE) T.BishopOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the bishop is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += BishopBehindPawn;
            if (TRACE) T.BishopBehindPawn[US]++;
        }

        // Apply a bonus when controlling both central squares on a long diagonal
        if (   testBit(LONG_DIAGONALS & ~CENTER_SQUARES, sq)
            && several(bishopAttacks(sq, board->pieces[PAWN]) & CENTER_SQUARES)) {
            eval += BishopLongDiagonal;
            if (TRACE) T.BishopLongDiagonal[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += BishopMobility[count];
        if (TRACE) T.BishopMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
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

    ei->attackedBy[US][ROOK] = 0ull;

    // Evaluate each rook
    while (tempRooks) {

        // Pop off the next rook
        sq = poplsb(&tempRooks);
        if (TRACE) T.RookValue[US]++;
        if (TRACE) T.RookPSQT32[relativeSquare32(US, sq)][US]++;

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
            && relativeRankOf(US, ei->kingSquare[THEM]) >= 6) {
            eval += RookOnSeventh;
            if (TRACE) T.RookOnSeventh[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the rook
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += RookMobility[count];
        if (TRACE) T.RookMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
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
        if (TRACE) T.QueenPSQT32[relativeSquare32(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = queenAttacks(sq, board->colours[WHITE] | board->colours[BLACK]);
        ei->attackedBy2[US]       |= attacks & ei->attacked[US];
        ei->attacked[US]          |= attacks;
        ei->attackedBy[US][QUEEN] |= attacks;

        // Apply a bonus (or penalty) based on the mobility of the queen
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += QueenMobility[count];
        if (TRACE) T.QueenMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[QUEEN];
        }
    }

    return eval;
}

int evaluateKings(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int count, dist, blocked, eval = 0;

    uint64_t myPawns     = board->pieces[PAWN ] & board->colours[  US];
    uint64_t enemyPawns  = board->pieces[PAWN ] & board->colours[THEM];
    uint64_t enemyQueens = board->pieces[QUEEN] & board->colours[THEM];

    uint64_t defenders  = (board->pieces[PAWN  ] & board->colours[US])
                        | (board->pieces[KNIGHT] & board->colours[US])
                        | (board->pieces[BISHOP] & board->colours[US]);

    int kingSq = ei->kingSquare[US];
    if (TRACE) T.KingValue[US]++;
    if (TRACE) T.KingPSQT32[relativeSquare32(US, kingSq)][US]++;

    // Bonus for our pawns and minors sitting within our king area
    count = popcount(defenders & ei->kingAreas[US]);
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

        // Identify if there are pieces which can move to the checking squares safely.
        // We consider forking a Queen to be a safe check, even with our own Queen.
        uint64_t knightChecks = knightThreats & safe & ei->attackedBy[THEM][KNIGHT];
        uint64_t bishopChecks = bishopThreats & safe & ei->attackedBy[THEM][BISHOP];
        uint64_t rookChecks   = rookThreats   & safe & ei->attackedBy[THEM][ROOK  ];
        uint64_t queenChecks  = queenThreats  & safe & ei->attackedBy[THEM][QUEEN ];

        count  = ei->kingAttackersCount[THEM] * ei->kingAttackersWeight[THEM];

        count += KSAttackValue     * scaledAttackCounts
               + KSWeakSquares     * popcount(weak & ei->kingAreas[US])
               + KSFriendlyPawns   * popcount(myPawns & ei->kingAreas[US] & ~weak)
               + KSNoEnemyQueens   * !enemyQueens
               + KSSafeQueenCheck  * popcount(queenChecks)
               + KSSafeRookCheck   * popcount(rookChecks)
               + KSSafeBishopCheck * popcount(bishopChecks)
               + KSSafeKnightCheck * popcount(knightChecks)
               + KSAdjustment;

        // Convert safety to an MG and EG score, if we are unsafe
        if (count > 0) eval -= MakeScore(count * count / 720, count / 20);
    }

    // Everything else is stored in the Pawn King Table
    if (ei->pkentry != NULL) return eval;

    // Evaluate based on the number of files between our King and the nearest
    // file-wise pawn. If there is no pawn, kingPawnFileDistance() returns the
    // same distance for both sides causing this evaluation term to be neutral
    dist = kingPawnFileDistance(board->pieces[PAWN], kingSq);
    ei->pkeval[US] += KingPawnFileProximity[dist];
    if (TRACE) T.KingPawnFileProximity[dist][US]++;

    // Evaluate King Shelter & King Storm threat by looking at the file of our King,
    // as well as the adjacent files. When looking at pawn distances, we will use a
    // distance of 7 to denote a missing pawn, since distance 7 is not possible otherwise.
    for (int file = MAX(0, fileOf(kingSq) - 1); file <= MIN(FILE_NB - 1, fileOf(kingSq) + 1); file++) {

        // Find closest friendly pawn at or above our King on a given file
        uint64_t ours = myPawns & Files[file] & forwardRanksMasks(US, rankOf(kingSq));
        int ourDist = !ours ? 7 : abs(rankOf(kingSq) - rankOf(backmost(US, ours)));

        // Find closest enemy pawn at or above our King on a given file
        uint64_t theirs = enemyPawns & Files[file] & forwardRanksMasks(US, rankOf(kingSq));
        int theirDist = !theirs ? 7 : abs(rankOf(kingSq) - rankOf(backmost(US, theirs)));

        // Evaluate King Shelter using pawn distance. Use separate evaluation
        // depending on the file, and if we are looking at the King's file
        ei->pkeval[US] += KingShelter[file == fileOf(kingSq)][file][ourDist];
        if (TRACE) T.KingShelter[file == fileOf(kingSq)][file][ourDist][US]++;

        // Evaluate King Storm using enemy pawn distance. Use a separate evaluation
        // depending on the file, and if the opponent's pawn is blocked by our own
        blocked = (ourDist != 7 && (ourDist == theirDist - 1));
        ei->pkeval[US] += KingStorm[blocked][mirrorFile(file)][theirDist];
        if (TRACE) T.KingStorm[blocked][mirrorFile(file)][theirDist][US]++;
    }

    return eval;
}

int evaluatePassed(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, rank, dist, flag, canAdvance, safeAdvance, eval = 0;

    uint64_t bitboard;
    uint64_t myPassers = board->colours[US] & ei->passedPawns;
    uint64_t occupied  = board->colours[WHITE] | board->colours[BLACK];
    uint64_t tempPawns = myPassers;

    // Evaluate each passed pawn
    while (tempPawns) {

        // Pop off the next passed Pawn
        sq = poplsb(&tempPawns);
        rank = relativeRankOf(US, sq);
        bitboard = pawnAdvance(1ull << sq, 0ull, US);

        // Evaluate based on rank, ability to advance, and safety
        canAdvance = !(bitboard & occupied);
        safeAdvance = !(bitboard & ei->attacked[THEM]);
        eval += PassedPawn[canAdvance][safeAdvance][rank];
        if (TRACE) T.PassedPawn[canAdvance][safeAdvance][rank][US]++;

        // Short-circuit evaluation for additional passers on a file
        if (several(forwardFileMasks(US, sq) & myPassers)) continue;

        // Evaluate based on distance from our king
        dist = distanceBetween(sq, ei->kingSquare[US]);
        eval += dist * PassedFriendlyDistance[rank];
        if (TRACE) T.PassedFriendlyDistance[rank][US] += dist;

        // Evaluate based on distance from their king
        dist = distanceBetween(sq, ei->kingSquare[THEM]);
        eval += dist * PassedEnemyDistance[rank];
        if (TRACE) T.PassedEnemyDistance[rank][US] += dist;

        // Apply a bonus when the path to promoting is uncontested
        bitboard = forwardRanksMasks(US, rankOf(sq)) & Files[fileOf(sq)];
        flag = !(bitboard & (board->colours[THEM] | ei->attacked[THEM]));
        eval += flag * PassedSafePromotionPath;
        if (TRACE) T.PassedSafePromotionPath[US] += flag;
    }

    return eval;
}

int evaluateThreats(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const uint64_t Rank3Rel = US == WHITE ? RANK_3 : RANK_6;

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

    uint64_t weakMinors = (knights | bishops) & poorlyDefended;

    // A friendly minor or major is overloaded if attacked and defended by exactly one
    uint64_t overloaded = (knights | bishops | rooks | queens)
                        & ei->attacked[  US] & ~ei->attackedBy2[  US]
                        & ei->attacked[THEM] & ~ei->attackedBy2[THEM];

    // Look for enemy non-pawn pieces which we may threaten with a pawn advance.
    // Don't consider pieces we already threaten, pawn moves which would be countered
    // by a pawn capture, and squares which are completely unprotected by our pieces.
    uint64_t pushThreat  = pawnAdvance(pawns, occupied, US);
    pushThreat |= pawnAdvance(pushThreat & ~attacksByPawns & Rank3Rel, occupied, US);
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

    // Penalty for any minor threat against minor pieces
    count = popcount((knights | bishops) & attacksByMinors);
    eval += count * ThreatMinorAttackedByMinor;
    if (TRACE) T.ThreatMinorAttackedByMinor[US] += count;

    // Penalty for all major threats against poorly supported minors
    count = popcount(weakMinors & attacksByMajors);
    eval += count * ThreatMinorAttackedByMajor;
    if (TRACE) T.ThreatMinorAttackedByMajor[US] += count;

    // Penalty for pawn and minor threats against our rooks
    count = popcount(rooks & (attacksByPawns | attacksByMinors));
    eval += count * ThreatRookAttackedByLesser;
    if (TRACE) T.ThreatRookAttackedByLesser[US] += count;

    // Penalty for king threats against our poorly defended minors
    count = popcount(weakMinors & ei->attackedBy[THEM][KING]);
    eval += count * ThreatMinorAttackedByKing;
    if (TRACE) T.ThreatMinorAttackedByKing[US] += count;

    // Penalty for king threats against our poorly defended rooks
    count = popcount(rooks & poorlyDefended & ei->attackedBy[THEM][KING]);
    eval += count * ThreatRookAttackedByKing;
    if (TRACE) T.ThreatRookAttackedByKing[US] += count;

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

int evaluateSpace(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int count, eval = 0;

    uint64_t friendly = board->colours[  US];
    uint64_t enemy    = board->colours[THEM];

    // Squares we attack with more enemy attackers and no friendly pawn attacks
    uint64_t uncontrolled =   ei->attackedBy2[THEM] & ei->attacked[US]
                           & ~ei->attackedBy2[US  ] & ~ei->attackedBy[US][PAWN];

    // Penalty for restricted piece moves
    count = popcount(uncontrolled & (friendly | enemy));
    eval += count * SpaceRestrictPiece;
    if (TRACE) T.SpaceRestrictPiece[US] += count;

    count = popcount(uncontrolled & ~friendly & ~enemy);
    eval += count * SpaceRestrictEmpty;
    if (TRACE) T.SpaceRestrictEmpty[US] += count;

    // Bonus for uncontested central squares
    // This is mostly relevant in the opening and the early middlegame, while rarely correct
    // in the endgame where one rook or queen could control many uncontested squares.
    // Thus we don't apply this term when below a threshold of minors/majors count.
    if (      popcount(board->pieces[KNIGHT] | board->pieces[BISHOP])
        + 2 * popcount(board->pieces[ROOK  ] | board->pieces[QUEEN ]) > 12) {
        count = popcount(~ei->attacked[THEM] & (ei->attacked[US] | friendly) & CENTER_BIG);
        eval += count * SpaceCenterControl;
        if (TRACE) T.SpaceCenterControl[US] += count;
    }

    return eval;
}

int evaluateClosedness(EvalInfo *ei, Board *board) {

    int closedness, count, eval = 0;

    uint64_t white = board->colours[WHITE];
    uint64_t black = board->colours[BLACK];

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t rooks   = board->pieces[ROOK  ];

    // Compute Closedness factor for this position
    closedness = 1 * popcount(board->pieces[PAWN])
               + 3 * popcount(ei->rammedPawns[WHITE])
               - 4 * openFileCount(board->pieces[PAWN]);
    closedness = MAX(0, MIN(8, closedness / 3));

    // Evaluate Knights based on how Closed the position is
    count = popcount(white & knights) - popcount(black & knights);
    eval += count * ClosednessKnightAdjustment[closedness];
    if (TRACE) T.ClosednessKnightAdjustment[closedness][WHITE] += count;

    // Evaluate Rooks based on how Closed the position is
    count = popcount(white & rooks) - popcount(black & rooks);
    eval += count * ClosednessRookAdjustment[closedness];
    if (TRACE) T.ClosednessRookAdjustment[closedness][WHITE] += count;

    return eval;
}

int evaluateComplexity(EvalInfo *ei, Board *board, int eval) {

    // Adjust endgame evaluation based on features related to how
    // likely the stronger side is to convert the position.
    // More often than not, this is a penalty for drawish positions.

    (void) ei; // Silence compiler warning

    int complexity;
    int eg = ScoreEG(eval);
    int sign = (eg > 0) - (eg < 0);

    int pawnsOnBothFlanks = (board->pieces[PAWN] & LEFT_FLANK )
                         && (board->pieces[PAWN] & RIGHT_FLANK);

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    // Compute the initiative bonus or malus for the attacking side
    complexity =  ComplexityTotalPawns  * popcount(board->pieces[PAWN])
               +  ComplexityPawnFlanks  * pawnsOnBothFlanks
               +  ComplexityPawnEndgame * !(knights | bishops | rooks | queens)
               +  ComplexityAdjustment;

    if (TRACE) T.ComplexityTotalPawns[WHITE]  += sign * popcount(board->pieces[PAWN]);
    if (TRACE) T.ComplexityPawnFlanks[WHITE]  += sign * pawnsOnBothFlanks;
    if (TRACE) T.ComplexityPawnEndgame[WHITE] += sign * !(knights | bishops | rooks | queens);
    if (TRACE) T.ComplexityAdjustment[WHITE]  += sign;

    // Avoid changing which side has the advantage
    int v = sign * MAX(ScoreEG(complexity), -abs(eg));

    return MakeScore(0, v);
}

int evaluateScaleFactor(Board *board, int eval) {

    // Scale endgames based upon the remaining material. We check
    // for various Opposite Coloured Bishop cases, positions with
    // a lone Queen against multiple minor pieces and/or rooks, and
    // positions with a Lone minor that should not be winnable

    const uint64_t pawns   = board->pieces[PAWN  ];
    const uint64_t knights = board->pieces[KNIGHT];
    const uint64_t bishops = board->pieces[BISHOP];
    const uint64_t rooks   = board->pieces[ROOK  ];
    const uint64_t queens  = board->pieces[QUEEN ];

    const uint64_t minors  = knights | bishops;
    const uint64_t pieces  = knights | bishops | rooks;

    const uint64_t white   = board->colours[WHITE];
    const uint64_t black   = board->colours[BLACK];

    const uint64_t weak    = ScoreEG(eval) < 0 ? white : black;
    const uint64_t strong  = ScoreEG(eval) < 0 ? black : white;


    // Check for opposite coloured bishops
    if (   onlyOne(white & bishops)
        && onlyOne(black & bishops)
        && onlyOne(bishops & WHITE_SQUARES)) {

        // Scale factor for OCB + knights
        if ( !(rooks | queens)
            && onlyOne(white & knights)
            && onlyOne(black & knights))
            return SCALE_OCB_ONE_KNIGHT;

        // Scale factor for OCB + rooks
        if ( !(knights | queens)
            && onlyOne(white & rooks)
            && onlyOne(black & rooks))
            return SCALE_OCB_ONE_ROOK;

        // Scale factor for lone OCB
        if (!(knights | rooks | queens))
            return SCALE_OCB_BISHOPS_ONLY;
    }

    // Lone Queens are weak against multiple pieces
    if (onlyOne(queens) && several(pieces) && pieces == (weak & pieces))
        return SCALE_LONE_QUEEN;

    // Lone Minor vs King + Pawns should never be won
    if ((strong & minors) && popcount(strong) == 2)
        return SCALE_DRAW;

    // Scale up lone pieces with massive pawn advantages
    if (   !queens
        && !several(pieces & white)
        && !several(pieces & black)
        &&  popcount(strong & pawns) - popcount(weak & pawns) > 2)
        return SCALE_LARGE_PAWN_ADV;

    return SCALE_NORMAL;
}

void initEvalInfo(EvalInfo *ei, Board *board, PKTable *pktable) {

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];

    uint64_t pawns   = board->pieces[PAWN  ];
    uint64_t bishops = board->pieces[BISHOP] | board->pieces[QUEEN];
    uint64_t rooks   = board->pieces[ROOK  ] | board->pieces[QUEEN];
    uint64_t kings   = board->pieces[KING  ];

    // Save some general information about the pawn structure for later
    ei->pawnAttacks[WHITE]    = pawnAttackSpan(white & pawns, ~0ull, WHITE);
    ei->pawnAttacks[BLACK]    = pawnAttackSpan(black & pawns, ~0ull, BLACK);
    ei->pawnAttacksBy2[WHITE] = pawnAttackDouble(white & pawns, ~0ull, WHITE);
    ei->pawnAttacksBy2[BLACK] = pawnAttackDouble(black & pawns, ~0ull, BLACK);
    ei->rammedPawns[WHITE]    = pawnAdvance(black & pawns, ~(white & pawns), BLACK);
    ei->rammedPawns[BLACK]    = pawnAdvance(white & pawns, ~(black & pawns), WHITE);
    ei->blockedPawns[WHITE]   = pawnAdvance(white | black, ~(white & pawns), BLACK);
    ei->blockedPawns[BLACK]   = pawnAdvance(white | black, ~(black & pawns), WHITE);

    // Compute an area for evaluating our King's safety.
    // The definition of the King Area can be found in masks.c
    ei->kingSquare[WHITE] = getlsb(white & kings);
    ei->kingSquare[BLACK] = getlsb(black & kings);
    ei->kingAreas[WHITE] = kingAreaMasks(WHITE, ei->kingSquare[WHITE]);
    ei->kingAreas[BLACK] = kingAreaMasks(BLACK, ei->kingSquare[BLACK]);

    // Exclude squares attacked by our opponents, our blocked pawns, and our own King
    ei->mobilityAreas[WHITE] = ~(ei->pawnAttacks[BLACK] | (white & kings) | ei->blockedPawns[WHITE]);
    ei->mobilityAreas[BLACK] = ~(ei->pawnAttacks[WHITE] | (black & kings) | ei->blockedPawns[BLACK]);

    // Init part of the attack tables. By doing this step here, evaluatePawns()
    // can start by setting up the attackedBy2 table, since King attacks are resolved
    ei->attacked[WHITE] = ei->attackedBy[WHITE][KING] = kingAttacks(ei->kingSquare[WHITE]);
    ei->attacked[BLACK] = ei->attackedBy[BLACK][KING] = kingAttacks(ei->kingSquare[BLACK]);

    // For mobility, we allow bishops to attack through each other
    ei->occupiedMinusBishops[WHITE] = (white | black) ^ (white & bishops);
    ei->occupiedMinusBishops[BLACK] = (white | black) ^ (black & bishops);

    // For mobility, we allow rooks to attack through each other
    ei->occupiedMinusRooks[WHITE] = (white | black) ^ (white & rooks);
    ei->occupiedMinusRooks[BLACK] = (white | black) ^ (black & rooks);

    // Init all of the King Safety information
    ei->kingAttacksCount[WHITE]    = ei->kingAttacksCount[BLACK]    = 0;
    ei->kingAttackersCount[WHITE]  = ei->kingAttackersCount[BLACK]  = 0;
    ei->kingAttackersWeight[WHITE] = ei->kingAttackersWeight[BLACK] = 0;

    // Try to read a hashed Pawn King Eval. Otherwise, start from scratch
    ei->pkentry       =     pktable == NULL ? NULL : getPKEntry(pktable, board->pkhash);
    ei->passedPawns   = ei->pkentry == NULL ? 0ull : ei->pkentry->passed;
    ei->pkeval[WHITE] = ei->pkentry == NULL ? 0    : ei->pkentry->eval;
    ei->pkeval[BLACK] = ei->pkentry == NULL ? 0    : 0;
}

void initEval() {

    // Init a normalized 64-length PSQT for the evaluation which
    // combines the Piece Values with the original PSQT Values

    for (int sq = 0; sq < SQUARE_NB; sq++) {

        const int w32 = relativeSquare32(WHITE, sq);
        const int b32 = relativeSquare32(BLACK, sq);

        PSQT[WHITE_PAWN  ][sq] = + PawnValue   +   PawnPSQT32[w32];
        PSQT[WHITE_KNIGHT][sq] = + KnightValue + KnightPSQT32[w32];
        PSQT[WHITE_BISHOP][sq] = + BishopValue + BishopPSQT32[w32];
        PSQT[WHITE_ROOK  ][sq] = + RookValue   +   RookPSQT32[w32];
        PSQT[WHITE_QUEEN ][sq] = + QueenValue  +  QueenPSQT32[w32];
        PSQT[WHITE_KING  ][sq] = + KingValue   +   KingPSQT32[w32];

        PSQT[BLACK_PAWN  ][sq] = - PawnValue   -   PawnPSQT32[b32];
        PSQT[BLACK_KNIGHT][sq] = - KnightValue - KnightPSQT32[b32];
        PSQT[BLACK_BISHOP][sq] = - BishopValue - BishopPSQT32[b32];
        PSQT[BLACK_ROOK  ][sq] = - RookValue   -   RookPSQT32[b32];
        PSQT[BLACK_QUEEN ][sq] = - QueenValue  -  QueenPSQT32[b32];
        PSQT[BLACK_KING  ][sq] = - KingValue   -   KingPSQT32[b32];
    }
}
