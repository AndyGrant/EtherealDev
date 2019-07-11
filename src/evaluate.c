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

const int PawnValue   = S( 100, 100);
const int KnightValue = S( 414, 352);
const int BishopValue = S( 432, 362);
const int RookValue   = S( 620, 589);
const int QueenValue  = S(1174,1165);
const int KingValue   = S(   0,   0);

/* Piece Square Evaluation Terms */

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S( -17,   8), S(   4,   4), S(  -9,   6), S(  -5,   0),
    S( -19,   4), S( -10,   4), S(  -7,  -3), S(  -1, -11),
    S( -15,  11), S( -10,  10), S(  12, -10), S(   9, -19),
    S(  -5,  14), S(   2,  10), S(  -1,  -1), S(  10, -17),
    S(  -5,  29), S(   1,  25), S(   5,  17), S(  31,  -8),
    S( -16, -44), S( -62, -17), S(   0, -31), S(  38, -46),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -46, -27), S(  -7, -35), S( -12, -29), S(   1, -19),
    S(  -4, -22), S(   3, -14), S(   2, -30), S(  11, -20),
    S(   4, -26), S(  19, -25), S(  14, -18), S(  25,  -7),
    S(  12,   1), S(  20,   3), S(  26,  13), S(  28,  17),
    S(  18,  14), S(  23,   7), S(  35,  22), S(  27,  32),
    S( -20,  13), S(   2,  10), S(  29,  22), S(  27,  25),
    S(   5, -12), S(  -8,  -3), S(  24, -21), S(  39,  -7),
    S(-170, -17), S( -80,  -4), S(-109,  14), S( -31,  -5),
};

const int BishopPSQT32[32] = {
    S(  17, -21), S(  12, -21), S(  -9, -11), S(   8, -15),
    S(  28, -36), S(  21, -32), S(  20, -24), S(  11, -15),
    S(  14, -15), S(  26, -19), S(  16, -11), S(  20, -10),
    S(  12, -10), S(  16,  -6), S(  14,   0), S(  20,   3),
    S( -13,  10), S(  16,   2), S(   4,   9), S(   9,  14),
    S(  -2,   5), S(  -2,  12), S(  10,  10), S(  14,   7),
    S( -42,  12), S( -34,   9), S(  -7,   2), S( -21,   5),
    S( -41,   0), S( -44,   3), S( -85,  11), S( -88,  20),
};

const int RookPSQT32[32] = {
    S( -13, -27), S( -16, -20), S(  -4, -25), S(   4, -30),
    S( -52, -15), S( -14, -32), S( -10, -32), S(   1, -36),
    S( -29, -16), S(  -9, -13), S( -17, -19), S(  -1, -27),
    S( -17,  -6), S( -10,   0), S(  -7,  -4), S(   6, -10),
    S(  -4,   0), S(  10,  -2), S(  20,  -6), S(  36, -10),
    S( -17,  11), S(  22,  -3), S(  -2,  10), S(  29,  -6),
    S(   1,  -1), S( -16,   8), S(   7,  -2), S(  23,  -2),
    S(  26,  16), S(  20,  16), S(   4,  20), S(  14,  17),
};

const int QueenPSQT32[32] = {
    S(  22, -64), S(   1, -51), S(   7, -61), S(  13, -48),
    S(   7, -46), S(  19, -63), S(  22, -80), S(  12, -36),
    S(   3, -34), S(  18, -33), S(   1,  -9), S(   2, -14),
    S(   5, -20), S(   7,  -1), S(  -6,   2), S( -19,  40),
    S( -16,   1), S( -13,  23), S( -23,   2), S( -35,  46),
    S( -35,  15), S( -29,   7), S( -33,   4), S( -23,   5),
    S( -13,  13), S( -65,  46), S( -19,  -3), S( -51,  33),
    S( -22,   2), S(   5,  -7), S(  -4,  -8), S( -16,   2),
};

const int KingPSQT32[32] = {
    S(  38, -65), S(  36, -42), S( -13,  -6), S( -31, -12),
    S(  29, -21), S(  -4, -12), S( -35,  12), S( -50,  14),
    S(   9, -23), S(  14, -19), S(  16,   3), S(  -7,  17),
    S(   7, -31), S(  87, -32), S(  43,   6), S(  -3,  25),
    S(   7, -18), S(  99, -30), S(  50,  10), S(   6,  19),
    S(  46, -29), S( 123, -25), S(  97,   0), S(  42,   0),
    S(   4, -60), S(  45, -17), S(  30,   0), S(   8,  -8),
    S(   5,-112), S(  71, -65), S( -22, -23), S( -21, -18),
};

/* Pawn Evaluation Terms */

const int PawnCandidatePasser[2][RANK_NB] = {
   {S(   0,   0), S( -23,  -8), S( -12,  11), S( -15,  26),
    S(  -2,  53), S(  35,  59), S(   0,   0), S(   0,   0)},
   {S(   0,   0), S( -15,  18), S(  -4,  18), S(   5,  39),
    S(  17,  74), S(  28,  53), S(   0,   0), S(   0,   0)},
};

const int PawnIsolated = S(  -6,  -9);

const int PawnStacked = S( -15, -20);

const int PawnBackwards[2] = { S(   6,   0), S(  -6, -17) };

const int PawnConnected32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(  -2,  -7), S(  10,   0), S(   3,   1), S(   5,  13),
    S(  12,  -1), S(  25,  -2), S(  19,   4), S(  21,  11),
    S(   8,  -1), S(  21,   3), S(   9,   8), S(  14,  15),
    S(  11,   7), S(  19,  11), S(  23,  18), S(  28,  15),
    S(  53,  17), S(  41,  40), S(  60,  43), S(  71,  48),
    S( 112,  -6), S( 204,  -1), S( 227,  27), S( 237,  41),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

/* Knight Evaluation Terms */

const int KnightOutpost[2] = { S(   7, -24), S(  29,  -5) };

const int KnightBehindPawn = S(   4,  18);

const int KnightMobility[9] = {
    S( -79,-107), S( -41,-105), S( -28, -56), S( -16, -35),
    S(  -6, -26), S(   0, -11), S(   8, -10), S(  17, -11),
    S(  28, -26),
};

/* Bishop Evaluation Terms */

const int BishopPair = S(  22,  59);

const int BishopRammedPawns = S(  -9, -12);

const int BishopOutpost[2] = { S(  11, -12), S(  37,  -2) };

const int BishopBehindPawn = S(   2,  16);

const int BishopMobility[14] = {
    S( -71,-147), S( -37, -99), S( -18, -62), S(  -9, -39),
    S(   1, -29), S(   9, -16), S(  12,  -7), S(  12,  -2),
    S(  11,   3), S(  15,   4), S(  14,   4), S(  34,  -7),
    S(  37,   3), S(  69, -32),
};

/* Rook Evaluation Terms */

const int RookFile[2] = { S(  12,   3), S(  31,   4) };

const int RookOnSeventh = S(  -6,  23);

const int RookMobility[15] = {
    S(-148,-113), S( -61,-114), S( -25, -74), S( -17, -37),
    S( -17, -18), S( -17,  -5), S( -17,   4), S( -11,   6),
    S(  -4,   9), S(   0,  13), S(   2,  18), S(   8,  20),
    S(   9,  23), S(  21,  13), S(  84, -28),
};

/* Queen Evaluation Terms */

const int QueenMobility[28] = {
    S( -61,-263), S(-210,-387), S( -58,-200), S( -36,-191),
    S( -23,-140), S( -20, -87), S( -15, -67), S( -15, -42),
    S(  -9, -43), S(  -8, -24), S(  -5, -20), S(  -4,  -5),
    S(  -2, -13), S(   0,  -4), S(  -2,  -3), S(  -6,   2),
    S(  -6,   0), S( -14,   2), S( -13,  -3), S( -18,  -9),
    S(  -9, -33), S(   4, -55), S(   5, -80), S(   9, -94),
    S(  -4,-101), S(  13,-121), S( -59, -45), S( -37, -73),
};

/* King Evaluation Terms */

const int KingDefenders[12] = {
    S( -21,   3), S(  -4,  -2), S(   2,   3), S(   8,   5),
    S(  16,   5), S(  25,   3), S(  28,  -4), S(  12,   0),
    S(  12,   6), S(  12,   6), S(  12,   6), S(  12,   6),
};

const int KingShelter[2][FILE_NB][RANK_NB] = {
  {{S(  -8,   1), S(  12, -24), S(  19,  -8), S(  12,   3),
    S(   9,   4), S(  -4,   1), S(  -3, -33), S( -44,  16)},
   {S(  16,  -7), S(  16, -15), S(   1,  -4), S( -13,   4),
    S( -27,  14), S( -72,  64), S(  90,  80), S( -16,   0)},
   {S(  32,  -4), S(  12,  -9), S( -27,   6), S( -10,  -9),
    S( -15,  -6), S( -16,   0), S(  -1,  66), S(  -9,  -2)},
   {S(   3,   9), S(  21, -12), S(   4, -11), S(  14, -21),
    S(  25, -35), S( -56,   9), S(-136,  51), S(   6,  -7)},
   {S( -15,   7), S(   4,  -3), S( -24,   2), S( -16,   5),
    S( -16,  -7), S( -46,  -4), S(  32, -19), S(  -2,  -2)},
   {S(  43, -17), S(  20, -16), S( -19,   2), S( -11, -18),
    S(   5, -23), S(  16, -17), S(  41, -31), S( -20,   1)},
   {S(  22, -13), S(  -3, -14), S( -22,  -2), S( -19,  -6),
    S( -28,  -5), S( -40,  29), S(   0,  44), S(  -6,   0)},
   {S(  -8, -11), S(   4, -17), S(   6,   2), S(  -3,  11),
    S( -13,  16), S( -12,  35), S(-189,  87), S( -15,  13)}},
  {{S(   0,   0), S( -12, -23), S(   7, -21), S( -38,  14),
    S( -27,   3), S(   3,  42), S(-167,  -7), S( -40,   8)},
   {S(   0,   0), S(  20, -20), S(   6,  -7), S( -18,   1),
    S(  -1, -13), S(  22,  60), S(-184,  -3), S( -30,   2)},
   {S(   0,   0), S(  25, -12), S(  -3,  -9), S(   4, -18),
    S(  15,  -7), S( -83,  47), S( -84, -72), S( -17,  -6)},
   {S(   0,   0), S(  -6,   7), S(  -4,  -1), S( -20,   4),
    S( -26,   1), S( -98,  31), S(   7, -41), S( -16,  -8)},
   {S(   0,   0), S(  12,   1), S(  10,  -5), S(  13, -11),
    S(  15, -26), S( -56,  16), S(-103, -60), S(   1,  -7)},
   {S(   0,   0), S(   1,  -7), S( -21,  -1), S( -23,  -8),
    S(  17, -23), S( -36,   4), S(  55,  37), S( -11,  -7)},
   {S(   0,   0), S(  18, -15), S(   9, -13), S( -10,  -5),
    S( -26,  11), S( -10,  17), S( -56, -47), S( -26,   9)},
   {S(   0,   0), S(   9, -35), S(  17, -26), S( -18,  -5),
    S( -20,  16), S( -10,  22), S(-227, -54), S( -19,   1)}},
};

const int KingStorm[2][FILE_NB/2][RANK_NB] = {
  {{S( -12,  29), S( 113, -10), S( -22,  24), S( -16,   7),
    S( -13,   2), S(  -8,  -4), S( -16,   4), S( -22,   0)},
   {S( -10,  48), S(  56,   8), S( -15,  21), S(  -3,  10),
    S(  -4,   5), S(   4,  -3), S(  -2,   2), S( -11,   1)},
   {S(   1,  38), S(  14,  21), S( -16,  18), S(  -9,   8),
    S(   3,   2), S(   6,   0), S(   9,  -5), S(   4,   0)},
   {S(  -7,  25), S(  10,  21), S( -15,   8), S( -12,   1),
    S( -12,   2), S(   8, -11), S(   3,  -9), S( -10,   2)}},
  {{S(   0,   0), S( -20, -20), S( -16,   1), S(  16, -15),
    S(   6,  -4), S(   5, -17), S(   2,   1), S(  19,  31)},
   {S(   0,   0), S( -19, -40), S(   1,  -6), S(  35, -10),
    S(   0,  -3), S(  12, -20), S(  -4,  -8), S( -19,   1)},
   {S(   0,   0), S( -31, -55), S( -21,  -5), S(  12, -10),
    S(   4,  -1), S(  -7, -12), S( -11, -12), S( -13,   5)},
   {S(   0,   0), S(  -4, -22), S( -12, -15), S( -10,  -3),
    S(  -3,  -6), S(   4, -22), S(  72, -15), S(  14,  20)}},
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
  {{S(   0,   0), S( -40,   4), S( -57,  18), S( -78,  25),
    S(  -4,   9), S(  79, -10), S( 161,  45), S(   0,   0)},
   {S(   0,   0), S( -36,  11), S( -60,  27), S( -71,  31),
    S( -11,  30), S(  94,  28), S( 183,  86), S(   0,   0)}},
  {{S(   0,   0), S( -29,  19), S( -56,  20), S( -70,  31),
    S(  -3,  34), S(  92,  33), S( 251,  91), S(   0,   0)},
   {S(   0,   0), S( -36,  17), S( -54,  20), S( -66,  37),
    S(   0,  44), S( 107,  88), S( 168, 215), S(   0,   0)}},
};

const int PassedFriendlyDistance[RANK_NB] = {
    S(   0,   0), S(   1,   0), S(   3,  -3), S(   6,  -8),
    S(   5, -11), S( -10, -10), S( -17,  -4), S(   0,   0),
};

const int PassedEnemyDistance[RANK_NB] = {
    S(   0,   0), S(   4,   0), S(   7,   1), S(  10,   6),
    S(   2,  16), S(   7,  22), S(  22,  20), S(   0,   0),
};

const int PassedSafePromotionPath = S( -19,  29);

/* Threat Evaluation Terms */

const int ThreatWeakPawn             = S( -12, -25);
const int ThreatMinorAttackedByPawn  = S( -46, -51);
const int ThreatMinorAttackedByMinor = S( -24, -33);
const int ThreatMinorAttackedByMajor = S( -22, -42);
const int ThreatRookAttackedByLesser = S( -43, -22);
const int ThreatQueenAttackedByOne   = S( -32, -48);
const int ThreatOverloadedPieces     = S(  -7, -11);
const int ThreatByPawnPush           = S(  13,  18);

/* General Evaluation Terms */

const int Tempo = 20;

#undef S

int evaluateBoard(Board *board, PKTable *pktable) {

    EvalInfo ei;
    int phase, factor, eval, pkeval;

    // Setup and perform all evaluations
    initEvalInfo(&ei, board, pktable);
    eval   = evaluatePieces(&ei, board);
    pkeval = ei.pkeval[WHITE] - ei.pkeval[BLACK];
    eval  += pkeval + board->psqtmat;

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

        uint64_t stoppers    = enemyPawns & passedPawnMasks(US, sq);
        uint64_t threats     = enemyPawns & pawnAttacks(US, sq);
        uint64_t support     = myPawns    & pawnAttacks(THEM, sq);
        uint64_t pushThreats = enemyPawns & pawnAttacks(US, sq + Forward);
        uint64_t pushSupport = myPawns    & pawnAttacks(THEM, sq + Forward);
        uint64_t leftovers   = stoppers ^ threats ^ pushThreats;

        // Save passed pawn information for later evaluation
        if (!stoppers) setBit(&ei->passedPawns, sq);

        // Apply a bonus for pawns which will become passers by advancing a single
        // square when exchanging our supporters with the remaining passer stoppers
        else if (!leftovers && popcount(pushSupport) >= popcount(pushThreats)) {
            flag = popcount(support) >= popcount(threats);
            pkeval += PawnCandidatePasser[flag][relativeRankOf(US, sq)];
            if (TRACE) T.PawnCandidatePasser[flag][relativeRankOf(US, sq)][US]++;
        }

        // Apply a penalty if the pawn is isolated, and there is not an
        // immediate pawn capture to potentially remedy the isolation
        if (!threats && !(adjacentFilesMasks(fileOf(sq)) & myPawns)) {
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
            flag = !(Files[fileOf(sq)] & enemyPawns);
            pkeval += PawnBackwards[flag];
            if (TRACE) T.PawnBackwards[flag][US]++;
        }

        // Apply a bonus if the pawn is connected and not backward
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
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += KnightOutpost[defended];
            if (TRACE) T.KnightOutpost[defended][US]++;
        }

        // Apply a bonus if the knight is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += KnightBehindPawn;
            if (TRACE) T.KnightBehindPawn[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += KnightMobility[count];
        if (TRACE) T.KnightMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM])) {
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
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += BishopOutpost[defended];
            if (TRACE) T.BishopOutpost[defended][US]++;
        }

        // Apply a bonus if the bishop is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += BishopBehindPawn;
            if (TRACE) T.BishopBehindPawn[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += BishopMobility[count];
        if (TRACE) T.BishopMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM])) {
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
        if ((attacks &= ei->kingAreas[THEM])) {
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
        if ((attacks &= ei->kingAreas[THEM])) {
            ei->kingAttacksCount[US] += popcount(attacks);
            ei->kingAttackersCount[US] += 1;
            ei->kingAttackersWeight[US] += KSAttackWeight[QUEEN];
        }
    }

    return eval;
}

int evaluateKings(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int count, eval = 0;

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

    // King Shelter & King Storm are stored in the Pawn King Table
    if (ei->pkentry != NULL) return eval;

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

        // Evaluate King Shelter using pawn distance. Use seperate evaluation
        // depending on the file, and if we are looking at the King's file
        ei->pkeval[US] += KingShelter[file == fileOf(kingSq)][file][ourDist];
        if (TRACE) T.KingShelter[file == fileOf(kingSq)][file][ourDist][US]++;

        // Evaluate King Storm using enemy pawn distance. Use a seperate evaluation
        // depending on the file, and if the opponent's pawn is blocked by our own
        int blocked = (ourDist != 7 && (ourDist == theirDist - 1));
        ei->pkeval[US] += KingStorm[blocked][mirrorFile(file)][theirDist];
        if (TRACE) T.KingStorm[blocked][mirrorFile(file)][theirDist][US]++;
    }

    return eval;
}

int evaluatePassed(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, rank, dist, flag, canAdvance, safeAdvance, eval = 0;

    uint64_t bitboard;
    uint64_t tempPawns = board->colours[US] & ei->passedPawns;
    uint64_t occupied  = board->colours[WHITE] | board->colours[BLACK];

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

    // Scale endgames based on remaining material. Currently, we only
    // look for OCB endgames that include only one Knight or one Rook

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];
    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    if (   onlyOne(white & bishops)
        && onlyOne(black & bishops)
        && onlyOne(bishops & WHITE_SQUARES)) {

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
    }

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
    ei->pawnAttacks[WHITE]  = pawnAttackSpan(white & pawns, ~0ull, WHITE);
    ei->pawnAttacks[BLACK]  = pawnAttackSpan(black & pawns, ~0ull, BLACK);
    ei->rammedPawns[WHITE]  = pawnAdvance(black & pawns, ~(white & pawns), BLACK);
    ei->rammedPawns[BLACK]  = pawnAdvance(white & pawns, ~(black & pawns), WHITE);
    ei->blockedPawns[WHITE] = pawnAdvance(white | black, ~(white & pawns), BLACK);
    ei->blockedPawns[BLACK] = pawnAdvance(white | black, ~(black & pawns), WHITE);

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

    // For mobility, we allow bishops to attack through eachother
    ei->occupiedMinusBishops[WHITE] = (white | black) ^ (white & bishops);
    ei->occupiedMinusBishops[BLACK] = (white | black) ^ (black & bishops);

    // For mobility, we allow rooks to attack through eachother
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
