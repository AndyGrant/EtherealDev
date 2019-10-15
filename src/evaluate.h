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

#pragma once

#include <stdint.h>

#include "types.h"

#ifdef TUNE
    #define TRACE (1)
#else
    #define TRACE (0)
#endif

enum {
    SCALE_OCB_BISHOPS_ONLY =  64,
    SCALE_OCB_ONE_KNIGHT   = 106,
    SCALE_OCB_ONE_ROOK     =  96,
    SCALE_NORMAL           = 128,
};

struct EvalTrace {
    double PawnValue[COLOUR_NB];
    double KnightValue[COLOUR_NB];
    double BishopValue[COLOUR_NB];
    double RookValue[COLOUR_NB];
    double QueenValue[COLOUR_NB];
    double KingValue[COLOUR_NB];
    double PawnPSQT32[32][COLOUR_NB];
    double KnightPSQT32[32][COLOUR_NB];
    double BishopPSQT32[32][COLOUR_NB];
    double RookPSQT32[32][COLOUR_NB];
    double QueenPSQT32[32][COLOUR_NB];
    double KingPSQT32[32][COLOUR_NB];
    double PawnCandidatePasser[2][8][COLOUR_NB];
    double PawnIsolated[COLOUR_NB];
    double PawnStacked[COLOUR_NB];
    double PawnBackwards[2][COLOUR_NB];
    double PawnConnected32[32][COLOUR_NB];
    double KnightOutpost[2][COLOUR_NB];
    double KnightBehindPawn[COLOUR_NB];
    double KnightMobility[9][COLOUR_NB];
    double BishopPair[COLOUR_NB];
    double BishopRammedPawns[COLOUR_NB];
    double BishopOutpost[2][COLOUR_NB];
    double BishopBehindPawn[COLOUR_NB];
    double BishopMobility[14][COLOUR_NB];
    double RookFile[2][COLOUR_NB];
    double RookOnSeventh[COLOUR_NB];
    double RookMobility[15][COLOUR_NB];
    double QueenMobility[28][COLOUR_NB];
    double KingPawnFileProximity[8][COLOUR_NB];
    double KingDefenders[12][COLOUR_NB];
    double KingShelter[2][8][8][COLOUR_NB];
    double KingStorm[2][4][8][COLOUR_NB];
    double KSAttackValue[COLOUR_NB];
    double KSWeakSquares[COLOUR_NB];
    double KSFriendlyPawns[COLOUR_NB];
    double KSNoEnemyQueens[COLOUR_NB];
    double KSSafeQueenCheck[COLOUR_NB];
    double KSSafeRookCheck[COLOUR_NB];
    double KSSafeBishopCheck[COLOUR_NB];
    double KSSafeKnightCheck[COLOUR_NB];
    double KSAdjustment[COLOUR_NB];
    double PassedPawn[2][2][8][COLOUR_NB];
    double PassedFriendlyDistance[8][COLOUR_NB];
    double PassedEnemyDistance[8][COLOUR_NB];
    double PassedSafePromotionPath[COLOUR_NB];
    double ThreatWeakPawn[COLOUR_NB];
    double ThreatMinorAttackedByPawn[COLOUR_NB];
    double ThreatMinorAttackedByMinor[COLOUR_NB];
    double ThreatMinorAttackedByMajor[COLOUR_NB];
    double ThreatRookAttackedByLesser[COLOUR_NB];
    double ThreatQueenAttackedByOne[COLOUR_NB];
    double ThreatOverloadedPieces[COLOUR_NB];
    double ThreatByPawnPush[COLOUR_NB];
    double ComplexityTotalPawns[COLOUR_NB];
    double ComplexityPawnFlanks[COLOUR_NB];
    double ComplexityPawnEndgame[COLOUR_NB];
    double ComplexityAdjustment[COLOUR_NB];
};

struct EvalInfo {
    uint64_t pawnAttacks[COLOUR_NB];
    uint64_t rammedPawns[COLOUR_NB];
    uint64_t blockedPawns[COLOUR_NB];
    uint64_t kingAreas[COLOUR_NB];
    uint64_t mobilityAreas[COLOUR_NB];
    uint64_t attacked[COLOUR_NB];
    uint64_t attackedBy2[COLOUR_NB];
    uint64_t attackedBy[COLOUR_NB][PIECE_NB];
    uint64_t occupiedMinusBishops[COLOUR_NB];
    uint64_t occupiedMinusRooks[COLOUR_NB];
    uint64_t passedPawns;
    int kingSquare[COLOUR_NB];
    int kingAttacksCount[COLOUR_NB];
    int kingAttackersCount[COLOUR_NB];
    int kingAttackersWeight[COLOUR_NB];
    int pkeval[COLOUR_NB];
    PKEntry *pkentry;
};

int evaluateBoard(Board *board, PKTable *pktable);
int evaluatePieces(EvalInfo *ei, Board *board);
int evaluatePawns(EvalInfo *ei, Board *board, int colour);
int evaluateKnights(EvalInfo *ei, Board *board, int colour);
int evaluateBishops(EvalInfo *ei, Board *board, int colour);
int evaluateRooks(EvalInfo *ei, Board *board, int colour);
int evaluateQueens(EvalInfo *ei, Board *board, int colour);
int evaluateKings(EvalInfo *ei, Board *board, int colour);
int evaluatePassed(EvalInfo *ei, Board *board, int colour);
int evaluateThreats(EvalInfo *ei, Board *board, int colour);
int evaluateScaleFactor(Board *board);
int evaluateComplexity(EvalInfo *ei, Board *board, int eval);
void initEvalInfo(EvalInfo *ei, Board *board, PKTable *pktable);
void initEval();

#define MakeScore(mg, eg) ((int)((unsigned int)(eg) << 16) + (mg))
#define ScoreMG(s) ((int16_t)((uint16_t)((unsigned)((s)))))
#define ScoreEG(s) ((int16_t)((uint16_t)((unsigned)((s) + 0x8000) >> 16)))

extern int PSQT[32][SQUARE_NB];
extern const int Tempo;
