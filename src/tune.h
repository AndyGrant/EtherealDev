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

#ifdef TUNE

#pragma once

#include "types.h"

/* General Tuner Settings */

#define KITERATIONS    (     10) // Decimal places to take computeOptimalK() to
#define NPARTITIONS    (     64) // How many split points to make for OpenMP
#define REPORTRATE     (     25) // Number of tuning epochs between each report
#define NLINEARTERMS   (    588) // Number of linear terms to be tuned (Total is 588)
#define NSAFETYTERMS   (     15) // Number of King Safety terms (Total is 15)
#define LEARNINGRATE   (    1.0) // Learning rate for each epoch's update
#define LRDROPDRATE    (   1.25) // Cut the learning rate by this after each regression
#define BATCHSIZE      (   8192) // Number of positions to work with per mini-batch
#define NPOSITIONS     (7500000) // Total number of FENS provided in FENFILENAME
#define FENFILENAME    ( "FENS") // Name of the file containing the FENs & Results
#define TUPLESTACKSIZE ((int)((double) NPOSITIONS * NLINEARTERMS / 64))

/* Enable or Disable Specific Linear Evaluation Terms for Tuning */

#define TunePawnValue                   (1)
#define TuneKnightValue                 (1)
#define TuneBishopValue                 (1)
#define TuneRookValue                   (1)
#define TuneQueenValue                  (1)
#define TuneKingValue                   (1)
#define TunePawnPSQT32                  (1)
#define TuneKnightPSQT32                (1)
#define TuneBishopPSQT32                (1)
#define TuneRookPSQT32                  (1)
#define TuneQueenPSQT32                 (1)
#define TuneKingPSQT32                  (1)
#define TunePawnCandidatePasser         (1)
#define TunePawnIsolated                (1)
#define TunePawnStacked                 (1)
#define TunePawnBackwards               (1)
#define TunePawnConnected32             (1)
#define TuneKnightOutpost               (1)
#define TuneKnightBehindPawn            (1)
#define TuneKnightMobility              (1)
#define TuneBishopPair                  (1)
#define TuneBishopRammedPawns           (1)
#define TuneBishopOutpost               (1)
#define TuneBishopBehindPawn            (1)
#define TuneBishopMobility              (1)
#define TuneRookFile                    (1)
#define TuneRookOnSeventh               (1)
#define TuneRookMobility                (1)
#define TuneQueenMobility               (1)
#define TuneKingDefenders               (1)
#define TuneKingShelter                 (1)
#define TuneKingStorm                   (1)
#define TunePassedPawn                  (1)
#define TunePassedFriendlyDistance      (1)
#define TunePassedEnemyDistance         (1)
#define TunePassedSafePromotionPath     (1)
#define TuneThreatWeakPawn              (1)
#define TuneThreatMinorAttackedByPawn   (1)
#define TuneThreatMinorAttackedByMinor  (1)
#define TuneThreatMinorAttackedByMajor  (1)
#define TuneThreatRookAttackedByLesser  (1)
#define TuneThreatQueenAttackedByOne    (1)
#define TuneThreatOverloadedPieces      (1)
#define TuneThreatByPawnPush            (1)

/* Enable or Disable Specific King Safety Evaluation Terms for Tuning */

#define TuneKSAttackWeight              (1)
#define TuneKSAttackValue               (1)
#define TuneKSWeakSquares               (1)
#define TuneKSFriendlyPawns             (1)
#define TuneKSNoEnemyQueens             (1)
#define TuneKSSafeQueenCheck            (1)
#define TuneKSSafeRookCheck             (1)
#define TuneKSSafeBishopCheck           (1)
#define TuneKSSafeKnightCheck           (1)
#define TuneKSAdjustment                (1)

/* End of Accessible Tuner Settings */

typedef double LinearVector[NLINEARTERMS][PHASE_NB];

typedef struct LinearTuple {
    int index, coeff;
} LinearTuple;

typedef double SafetyVector[NSAFETYTERMS];

typedef double SafetyTuple[2];

typedef struct TuningEntry {
    double result, turn;
    double eval, linearEval, scaleFactor;
    double phase, phaseFactors[PHASE_NB];
    int ntuples; LinearTuple *linear;
    SafetyTuple safety[NSAFETYTERMS];
} TuningEntry;

// Init Linear & King Safety Params with an N dimensional array

#define INIT_LINEAR_PARAM_0(TERM, PARAMS) do {                  \
     PARAMS[i  ][MG] = ScoreMG(TERM);                           \
     PARAMS[i++][EG] = ScoreEG(TERM);                           \
} while (0)

#define INIT_LINEAR_PARAM_1(TERM, PARAMS, A) do {               \
    for (int _a = 0; _a < A; _a++)                              \
       {PARAMS[i  ][MG] = ScoreMG(TERM[_a]);                    \
        PARAMS[i++][EG] = ScoreEG(TERM[_a]);}                   \
} while (0)

#define INIT_LINEAR_PARAM_2(TERM, PARAMS, A, B) do {            \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_LINEAR_PARAM_1(TERM[_b], PARAMS, B);               \
} while (0)

#define INIT_LINEAR_PARAM_3(TERM, PARAMS, A, B, C) do {         \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_LINEAR_PARAM_2(TERM[_c], PARAMS, B, C);            \
} while (0)

#define INIT_SAFETY_PARAM_0(TERM, PARAMS) do {                  \
     PARAMS[i++] = TERM;                                        \
} while (0)

#define INIT_SAFETY_PARAM_1(TERM, PARAMS, A) do {               \
    for (int _a = 0; _a < A; _a++)                              \
       PARAMS[i++] = TERM[_a];                                  \
} while (0)


// Init Linear & King Safety Coeffs with an N dimensional array

#define INIT_LINEAR_COEFF_0(TERM, COEFFS) do {                  \
    COEFFS[i++] = T.TERM[WHITE] - T.TERM[BLACK];                \
} while (0)

#define INIT_LINEAR_COEFF_1(TERM, COEFFS, A) do {               \
    for (int _a = 0; _a < A; _a++)                              \
        COEFFS[i++] = T.TERM[_a][WHITE] - T.TERM[_a][BLACK];    \
} while (0)

#define INIT_LINEAR_COEFF_2(TERM, COEFFS, A, B) do {            \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_LINEAR_COEFF_1(TERM[_b], COEFFS, B);               \
} while (0)

#define INIT_LINEAR_COEFF_3(TERM, COEFFS, A, B, C) do {         \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_LINEAR_COEFF_2(TERM[_c], COEFFS, B, C);            \
} while (0)

#define INIT_SAFETY_COEFF_0(TERM, COEFFS) do {                  \
    COEFFS[i  ][WHITE] = T.TERM[WHITE];                         \
    COEFFS[i++][BLACK] = T.TERM[BLACK];                         \
} while (0)

#define INIT_SAFETY_COEFF_1(TERM, COEFFS, A) do {               \
    for (int _a = 0; _a < A; _a++)                              \
      { COEFFS[i  ][WHITE] = T.TERM[_a][WHITE];                 \
        COEFFS[i++][BLACK] = T.TERM[_a][BLACK]; }               \
} while (0)

// Wrap all the printing methods for use by EXCUTE_ON_TERMS

#define PRINT_LINEAR_PARAM_0(TERM, PARAMS) do {                 \
    printLinearParameters_0(#TERM, PARAMS);                     \
} while (0)

#define PRINT_LINEAR_PARAM_1(TERM, PARAMS, A) do {              \
    printLinearParameters_1(#TERM, PARAMS, A);                  \
} while (0)

#define PRINT_LINEAR_PARAM_2(TERM, PARAMS, A, B) do {           \
    printLinearParameters_2(#TERM, PARAMS, A, B);               \
} while (0)

#define PRINT_LINEAR_PARAM_3(TERM, PARAMS, A, B, C) do {        \
    printLinearParameters_3(#TERM, PARAMS, A, B, C);            \
} while (0)

#define PRINT_SAFETY_PARAM_0(TERM, PARAMS) do {                 \
    printSafetyParameters_0(#TERM, PARAMS)                      \
} while (0)

#define PRINT_SAFETY_PARAM_1(TERM, PARAMS, A) do {              \
    printSafetyParameters_1(#TERM, PARAMS, A)                   \
} while (0)

// Wrap the Param, Coeff, and Prints to check for being enabled.
// King Safety params and coeffs are ALWAYS computed and stored,
// so that we can ALWAYS recompute the KS evaluation, so that we
// may only track deltas in the params when doing Linear terms

#define ENABLE_LINEAR_0(FNAME, TERM, ARRAY) do {                \
    if (Tune##TERM) FNAME##_0(TERM, ARRAY);                     \
} while (0)

#define ENABLE_LINEAR_1(FNAME, TERM, ARRAY, A) do {             \
    if (Tune##TERM) FNAME##_1(TERM, ARRAY, A);                  \
} while (0)

#define ENABLE_LINEAR_2(FNAME, TERM, ARRAY, A, B) do {          \
    if (Tune##TERM) FNAME##_2(TERM, ARRAY, A, B);               \
} while (0)

#define ENABLE_LINEAR_3(FNAME, TERM, ARRAY, A, B, C) do {       \
    if (Tune##TERM) FNAME##_3(TERM, ARRAY, A, B, C);            \
} while (0)

#define ENABLE_SAFETY_0(FNAME, TERM, ARRAY) do {                \
    FNAME##_0(TERM, ARRAY);                                     \
} while (0)

#define ENABLE_SAFETY_1(FNAME, TERM, ARRAY, A) do {             \
    FNAME##_1(TERM, ARRAY, A);                                  \
} while (0)


// Finally, wrap the above for both Linear and King Safety Tuning

#define EXECUTE_ON_LINEAR_TERMS(FNAME, ARRAY) do {              \
    ENABLE_LINEAR_0(FNAME, PawnValue, ARRAY);                   \
    ENABLE_LINEAR_0(FNAME, KnightValue, ARRAY);                 \
    ENABLE_LINEAR_0(FNAME, BishopValue, ARRAY);                 \
    ENABLE_LINEAR_0(FNAME, RookValue, ARRAY);                   \
    ENABLE_LINEAR_0(FNAME, QueenValue, ARRAY);                  \
    ENABLE_LINEAR_0(FNAME, KingValue, ARRAY);                   \
    ENABLE_LINEAR_1(FNAME, PawnPSQT32, ARRAY, 32);              \
    ENABLE_LINEAR_1(FNAME, KnightPSQT32, ARRAY, 32);            \
    ENABLE_LINEAR_1(FNAME, BishopPSQT32, ARRAY, 32);            \
    ENABLE_LINEAR_1(FNAME, RookPSQT32, ARRAY, 32);              \
    ENABLE_LINEAR_1(FNAME, QueenPSQT32, ARRAY, 32);             \
    ENABLE_LINEAR_1(FNAME, KingPSQT32, ARRAY, 32);              \
    ENABLE_LINEAR_2(FNAME, PawnCandidatePasser, ARRAY, 2, 8);   \
    ENABLE_LINEAR_0(FNAME, PawnIsolated, ARRAY);                \
    ENABLE_LINEAR_0(FNAME, PawnStacked, ARRAY);                 \
    ENABLE_LINEAR_1(FNAME, PawnBackwards, ARRAY, 2);            \
    ENABLE_LINEAR_1(FNAME, PawnConnected32, ARRAY, 32);         \
    ENABLE_LINEAR_1(FNAME, KnightOutpost, ARRAY, 2);            \
    ENABLE_LINEAR_0(FNAME, KnightBehindPawn, ARRAY);            \
    ENABLE_LINEAR_1(FNAME, KnightMobility, ARRAY, 9);           \
    ENABLE_LINEAR_0(FNAME, BishopPair, ARRAY);                  \
    ENABLE_LINEAR_0(FNAME, BishopRammedPawns, ARRAY);           \
    ENABLE_LINEAR_1(FNAME, BishopOutpost, ARRAY, 2);            \
    ENABLE_LINEAR_0(FNAME, BishopBehindPawn, ARRAY);            \
    ENABLE_LINEAR_1(FNAME, BishopMobility, ARRAY, 14);          \
    ENABLE_LINEAR_1(FNAME, RookFile, ARRAY, 2);                 \
    ENABLE_LINEAR_0(FNAME, RookOnSeventh, ARRAY);               \
    ENABLE_LINEAR_1(FNAME, RookMobility, ARRAY, 15);            \
    ENABLE_LINEAR_1(FNAME, QueenMobility, ARRAY, 28);           \
    ENABLE_LINEAR_1(FNAME, KingDefenders, ARRAY, 12);           \
    ENABLE_LINEAR_3(FNAME, KingShelter, ARRAY, 2, 8, 8);        \
    ENABLE_LINEAR_3(FNAME, KingStorm, ARRAY, 2, 4, 8);          \
    ENABLE_LINEAR_3(FNAME, PassedPawn, ARRAY, 2, 2, 8);         \
    ENABLE_LINEAR_1(FNAME, PassedFriendlyDistance, ARRAY, 8);   \
    ENABLE_LINEAR_1(FNAME, PassedEnemyDistance, ARRAY, 8);      \
    ENABLE_LINEAR_0(FNAME, PassedSafePromotionPath, ARRAY);     \
    ENABLE_LINEAR_0(FNAME, ThreatWeakPawn, ARRAY);              \
    ENABLE_LINEAR_0(FNAME, ThreatMinorAttackedByPawn, ARRAY);   \
    ENABLE_LINEAR_0(FNAME, ThreatMinorAttackedByMinor, ARRAY);  \
    ENABLE_LINEAR_0(FNAME, ThreatMinorAttackedByMajor, ARRAY);  \
    ENABLE_LINEAR_0(FNAME, ThreatRookAttackedByLesser, ARRAY);  \
    ENABLE_LINEAR_0(FNAME, ThreatQueenAttackedByOne, ARRAY);    \
    ENABLE_LINEAR_0(FNAME, ThreatOverloadedPieces, ARRAY);      \
    ENABLE_LINEAR_0(FNAME, ThreatByPawnPush, ARRAY);            \
} while (0)

#define EXECUTE_ON_SAFETY_TERMS(FNAME, ARRAY) do {              \
    ENABLE_SAFETY_1(FNAME, KSAttackWeight, ARRAY, 6);           \
    ENABLE_SAFETY_0(FNAME, KSAttackValue, ARRAY);               \
    ENABLE_SAFETY_0(FNAME, KSWeakSquares, ARRAY);               \
    ENABLE_SAFETY_0(FNAME, KSFriendlyPawns, ARRAY);             \
    ENABLE_SAFETY_0(FNAME, KSNoEnemyQueens, ARRAY);             \
    ENABLE_SAFETY_0(FNAME, KSSafeQueenCheck, ARRAY);            \
    ENABLE_SAFETY_0(FNAME, KSSafeRookCheck, ARRAY);             \
    ENABLE_SAFETY_0(FNAME, KSSafeBishopCheck, ARRAY);           \
    ENABLE_SAFETY_0(FNAME, KSSafeKnightCheck, ARRAY);           \
    ENABLE_SAFETY_0(FNAME, KSAdjustment, ARRAY);                \
} while (0)

void runTexelTuning(Thread *thread);
void initCurrentParameters(LinearVector cparams, SafetyVector ksparams);
void initTuningEntries(Thread *thread, TuningEntry *tes, SafetyVector ksparams);
void initCoefficientTuples(TuningEntry *te);

double linearEvaluation(TuningEntry *te, LinearVector params);
double safetyEvaluation(TuningEntry *te, SafetyVector ksparams, int colour);
double evaluate(TuningEntry *te, LinearVector params, SafetyVector ksparams);



#endif
