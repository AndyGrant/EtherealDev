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

#if defined(TUNE)

#pragma once

#include "types.h"

#define NPARTITIONS  (     64) // Total thread partitions
#define KPRECISION   (     10) // Iterations for computing K
#define REPORTING    (      1) // How often to report progress
#define NTERMS       (     13) // Total terms in the Tuner (660)
#define TUNE_SAFETY  (      1) // All Safety terms must be Tuned at once

#define LEARNING     (  100.0) // Learning rate
#define BATCHSIZE    (  32768) // FENs per mini-batch
#define NPOSITIONS   (5888224) // Total FENS in the book

#define STATICWEIGHT (   0.25) // Weight of the Static Evaluation
#define SEARCHWEIGHT (   0.75) // Weight of the Depth 10 Search

#define STACKSIZE ((int)((double) NPOSITIONS * NTERMS / 32))

#define TunePawnValue                   (0)
#define TuneKnightValue                 (0)
#define TuneBishopValue                 (0)
#define TuneRookValue                   (0)
#define TuneQueenValue                  (0)
#define TuneKingValue                   (0)
#define TunePawnPSQT32                  (0)
#define TuneKnightPSQT32                (0)
#define TuneBishopPSQT32                (0)
#define TuneRookPSQT32                  (0)
#define TuneQueenPSQT32                 (0)
#define TuneKingPSQT32                  (0)
#define TunePawnCandidatePasser         (0)
#define TunePawnIsolated                (0)
#define TunePawnStacked                 (0)
#define TunePawnBackwards               (0)
#define TunePawnConnected32             (0)
#define TuneKnightOutpost               (0)
#define TuneKnightBehindPawn            (0)
#define TuneKnightInSiberia             (0)
#define TuneKnightMobility              (0)
#define TuneBishopPair                  (0)
#define TuneBishopRammedPawns           (0)
#define TuneBishopOutpost               (0)
#define TuneBishopBehindPawn            (0)
#define TuneBishopLongDiagonal          (0)
#define TuneBishopMobility              (0)
#define TuneRookFile                    (0)
#define TuneRookOnSeventh               (0)
#define TuneRookMobility                (0)
#define TuneQueenMobility               (0)
#define TuneKingDefenders               (0)
#define TuneKingPawnFileProximity       (0)
#define TuneKingShelter                 (0)
#define TuneKingStorm                   (0)
#define TunePassedPawn                  (0)
#define TunePassedFriendlyDistance      (0)
#define TunePassedEnemyDistance         (0)
#define TunePassedSafePromotionPath     (0)
#define TuneThreatWeakPawn              (0)
#define TuneThreatMinorAttackedByPawn   (0)
#define TuneThreatMinorAttackedByMinor  (0)
#define TuneThreatMinorAttackedByMajor  (0)
#define TuneThreatRookAttackedByLesser  (0)
#define TuneThreatMinorAttackedByKing   (0)
#define TuneThreatRookAttackedByKing    (0)
#define TuneThreatQueenAttackedByOne    (0)
#define TuneThreatOverloadedPieces      (0)
#define TuneThreatByPawnPush            (0)
#define TuneSpaceRestrictPiece          (0)
#define TuneSpaceRestrictEmpty          (0)
#define TuneSpaceCenterControl          (0)
#define TuneClosednessKnightAdjustment  (0)
#define TuneClosednessRookAdjustment    (0)
#define TuneComplexityTotalPawns        (0)
#define TuneComplexityPawnFlanks        (0)
#define TuneComplexityPawnEndgame       (0)
#define TuneComplexityAdjustment        (0)

#define TuneSafetyKnightWeight          (TUNE_SAFETY)
#define TuneSafetyBishopWeight          (TUNE_SAFETY)
#define TuneSafetyRookWeight            (TUNE_SAFETY)
#define TuneSafetyQueenWeight           (TUNE_SAFETY)
#define TuneSafetyAttackValue           (TUNE_SAFETY)
#define TuneSafetyWeakSquares           (TUNE_SAFETY)
#define TuneSafetyFriendlyPawns         (TUNE_SAFETY)
#define TuneSafetyNoEnemyQueens         (TUNE_SAFETY)
#define TuneSafetySafeQueenCheck        (TUNE_SAFETY)
#define TuneSafetySafeRookCheck         (TUNE_SAFETY)
#define TuneSafetySafeBishopCheck       (TUNE_SAFETY)
#define TuneSafetySafeKnightCheck       (TUNE_SAFETY)
#define TuneSafetyAdjustment            (TUNE_SAFETY)

enum { NORMAL, MGONLY, EGONLY, SAFETY };

typedef struct TTuple {
    int index;
    int coeff;
} TTuple;

typedef struct TEntry {
    int ntuples;
    double result;
    double eval, phase;
    double factors[PHASE_NB];
    TTuple *tuples;
} TEntry;

typedef int TArray[NTERMS];
typedef double TVector[NTERMS][PHASE_NB];

#define PackCoeff(x)  ((int)((unsigned int)(x[BLACK]) << 16) + (x[WHITE]))
#define WhiteCoeff(x) ((int16_t)((uint16_t)((unsigned)((x)))))
#define BlackCoeff(x) ((int16_t)((uint16_t)((unsigned)((x) + 0x8000) >> 16)))

void runTuner(Thread *thread);
void initTunerEntries(TEntry *tes, Thread *thread);

void initTypeManager(TArray types);
void initCoefficients(TArray coeffs);
void initCurrentParameters(TVector cparams);

void updateMemory(TEntry *te, int size);
void updateGradient(TEntry *entries, TVector gradient, TVector params, TVector cparams, TArray types, double K, int batch);
double computeGradient(TEntry *entry, int index, int coeff, TArray types, int phase, int wsafety, int bsafety);

void shuffleTunerEntries(TEntry *tes);

double sigmoid(double K, double S);
double computeOptimalK(TEntry *tes);
double completeEvaluationError(TEntry *tes, double K);
double completeLinearError(TEntry *tes, TVector params, TVector cparams, TArray types, double K);

double linearEvaluation(TEntry *entry, TVector params, TVector cparams, TArray types);
double linearEvaluationNormal(TEntry *te, TVector params, TArray types);
double linearEvaluationSafety(TEntry *te, TVector params, TVector cparams, TArray types);
double linearSafetyMG(TEntry *entry, TVector params, TVector cparams, TArray types, int colour);

void printParameters(TVector params, TVector cparams);
void print_0(char *name, int params[NTERMS][PHASE_NB], int i, char *S);
void print_1(char *name, int params[NTERMS][PHASE_NB], int i, int A, char *S);
void print_2(char *name, int params[NTERMS][PHASE_NB], int i, int A, int B, char *S);
void print_3(char *name, int params[NTERMS][PHASE_NB], int i, int A, int B, int C, char *S);

// Initalize the Type Manger for an N dimensional array

#define INIT_TYPE_0(term, t, s) do {                                \
    types[i++] = t;                                                 \
} while (0)

#define INIT_TYPE_1(term, a, t, s) do {                             \
    for (int _a = 0; _a < a; _a++)                                  \
        types[i++] = t;                                             \
} while (0)

#define INIT_TYPE_2(term, a, b, t, s) do {                          \
    for (int _b = 0; _b < a; _b++)                                  \
        INIT_TYPE_1(term[_b], b, t, s);                             \
} while (0)

#define INIT_TYPE_3(term, a, b, c, t, s) do {                       \
    for (int _c = 0; _c < a; _c++)                                  \
        INIT_TYPE_2(term[_c], b, c, t, s);                          \
} while (0)

// Initalize Parameters of an N dimensional array

#define INIT_PARAM_0(term, t, s) do {                               \
     cparams[i  ][MG] = ScoreMG(term);                              \
     cparams[i++][EG] = ScoreEG(term);                              \
} while (0)

#define INIT_PARAM_1(term, a, t, s) do {                            \
    for (int _a = 0; _a < a; _a++)                                  \
       {cparams[i  ][MG] = ScoreMG(term[_a]);                       \
        cparams[i++][EG] = ScoreEG(term[_a]);}                      \
} while (0)

#define INIT_PARAM_2(term, a, b, t, s) do {                         \
    for (int _b = 0; _b < a; _b++)                                  \
        INIT_PARAM_1(term[_b], b, t, s);                            \
} while (0)

#define INIT_PARAM_3(term, a, b, c, t, s) do {                      \
    for (int _c = 0; _c < a; _c++)                                  \
        INIT_PARAM_2(term[_c], b, c, t, s);                         \
} while (0)

// Initalize Coefficients from an N dimensional array

#define INIT_COEFF_0(term, t, s) do {                               \
    if (t == SAFETY) coeffs[i++] = PackCoeff(T.term);               \
    else coeffs[i++] = T.term[WHITE] - T.term[BLACK];               \
} while (0)

#define INIT_COEFF_1(term, a, t, s) do {                            \
    for (int _a = 0; _a < a; _a++) {                                \
        if (t == SAFETY) coeffs[i++] = PackCoeff(T.term[_a]);       \
        else coeffs[i++] = T.term[_a][WHITE] - T.term[_a][BLACK];   \
    }                                                               \
} while (0)

#define INIT_COEFF_2(term, a, b, t, s) do {                         \
    for (int _b = 0; _b < a; _b++)                                  \
        INIT_COEFF_1(term[_b], b, t, s);                            \
} while (0)

#define INIT_COEFF_3(term, a, b, c, t, s) do {                      \
    for (int _c = 0; _c < a; _c++)                                  \
        INIT_COEFF_2(term[_c], b, c, t, s);                         \
} while (0)

// Print Parameters of an N dimensional array

#define PRINT_0(term, t, s) (print_0(#term, tparams, i, s), i+=1)

#define PRINT_1(term, a, t, s) (print_1(#term, tparams, i, a, s), i+=a)

#define PRINT_2(term, a, b, t, s) (print_2(#term, tparams, i, a, b, s), i+=a*b)

#define PRINT_3(term, a, b, c, t, s) (print_3(#term, tparams, i, a, b, c, s), i+=a*b*c)

// Generic wrapper for all of the above functions

#define ENABLE_0(f, term, t, s) do {                                \
    if (Tune##term) f##_0(term, t, s);                              \
} while (0)

#define ENABLE_1(f, term, a, t, s) do {                             \
    if (Tune##term) f##_1(term, a, t, s);                           \
} while (0)

#define ENABLE_2(f, term, a, b, t, s) do {                          \
    if (Tune##term) f##_2(term, a, b, t, s);                        \
} while (0)

#define ENABLE_3(f, term, a, b, c, t, s) do {                       \
    if (Tune##term) f##_3(term, a, b, c, t, s);                     \
} while (0)

// Configuration for each aspect of the evaluation terms

#define EXECUTE_ON_TERMS(f) do {                                            \
    ENABLE_0(f, PawnValue, NORMAL, "");                                     \
    ENABLE_0(f, KnightValue, NORMAL, "");                                   \
    ENABLE_0(f, BishopValue, NORMAL, "");                                   \
    ENABLE_0(f, RookValue, NORMAL, "");                                     \
    ENABLE_0(f, QueenValue, NORMAL, "");                                    \
    ENABLE_0(f, KingValue, NORMAL, "");                                     \
    ENABLE_1(f, PawnPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_1(f, KnightPSQT32, 32, NORMAL, "[32]");                          \
    ENABLE_1(f, BishopPSQT32, 32, NORMAL, "[32]");                          \
    ENABLE_1(f, RookPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_1(f, QueenPSQT32, 32, NORMAL, "[32]");                           \
    ENABLE_1(f, KingPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_2(f, PawnCandidatePasser, 2, 8, NORMAL, "[2][RANK_NB]");         \
    ENABLE_0(f, PawnIsolated, NORMAL, "");                                  \
    ENABLE_1(f, PawnStacked, 2, NORMAL, "[2]");                             \
    ENABLE_2(f, PawnBackwards, 2, 8, NORMAL, "[2][RANK_NB]");               \
    ENABLE_1(f, PawnConnected32, 32, NORMAL, "[32]");                       \
    ENABLE_2(f, KnightOutpost, 2, 2, NORMAL, "[2][2]");                     \
    ENABLE_0(f, KnightBehindPawn, NORMAL, "");                              \
    ENABLE_1(f, KnightInSiberia, 4, NORMAL, "[4]");                         \
    ENABLE_1(f, KnightMobility, 9, NORMAL, "[9]");                          \
    ENABLE_0(f, BishopPair, NORMAL, "");                                    \
    ENABLE_0(f, BishopRammedPawns, NORMAL, "");                             \
    ENABLE_2(f, BishopOutpost, 2, 2, NORMAL, "[2][2]");                     \
    ENABLE_0(f, BishopBehindPawn, NORMAL, "");                              \
    ENABLE_0(f, BishopLongDiagonal, NORMAL, "");                            \
    ENABLE_1(f, BishopMobility, 14, NORMAL, "[14]");                        \
    ENABLE_1(f, RookFile, 2, NORMAL, "[2]");                                \
    ENABLE_0(f, RookOnSeventh, NORMAL, "");                                 \
    ENABLE_1(f, RookMobility, 15, NORMAL, "[15]");                          \
    ENABLE_1(f, QueenMobility, 28, NORMAL, "[28]");                         \
    ENABLE_1(f, KingDefenders, 12, NORMAL, "[12]");                         \
    ENABLE_1(f, KingPawnFileProximity, 8, NORMAL, "[FILE_NB]");             \
    ENABLE_3(f, KingShelter, 2, 8, 8, NORMAL, "[2][FILE_NB][RANK_NB]");     \
    ENABLE_3(f, KingStorm, 2, 4, 8, NORMAL, "[2][FILE_NB/2][RANK_NB]");     \
    ENABLE_3(f, PassedPawn, 2, 2, 8, NORMAL, "[2][2][RANK_NB]");            \
    ENABLE_1(f, PassedFriendlyDistance, 8, NORMAL, "[FILE_NB]");            \
    ENABLE_1(f, PassedEnemyDistance, 8, NORMAL, "[FILE_NB]");               \
    ENABLE_0(f, PassedSafePromotionPath, NORMAL, "");                       \
    ENABLE_0(f, ThreatWeakPawn, NORMAL, "");                                \
    ENABLE_0(f, ThreatMinorAttackedByPawn, NORMAL, "");                     \
    ENABLE_0(f, ThreatMinorAttackedByMinor, NORMAL, "");                    \
    ENABLE_0(f, ThreatMinorAttackedByMajor, NORMAL, "");                    \
    ENABLE_0(f, ThreatRookAttackedByLesser, NORMAL, "");                    \
    ENABLE_0(f, ThreatMinorAttackedByKing, NORMAL, "");                     \
    ENABLE_0(f, ThreatRookAttackedByKing, NORMAL, "");                      \
    ENABLE_0(f, ThreatQueenAttackedByOne, NORMAL, "");                      \
    ENABLE_0(f, ThreatOverloadedPieces, NORMAL, "");                        \
    ENABLE_0(f, ThreatByPawnPush, NORMAL, "");                              \
    ENABLE_0(f, SpaceRestrictPiece, NORMAL, "");                            \
    ENABLE_0(f, SpaceRestrictEmpty, NORMAL, "");                            \
    ENABLE_0(f, SpaceCenterControl, NORMAL, "");                            \
    ENABLE_1(f, ClosednessKnightAdjustment, 9, NORMAL, "[9]");              \
    ENABLE_1(f, ClosednessRookAdjustment, 9, NORMAL, "[9]");                \
    ENABLE_0(f, ComplexityTotalPawns, EGONLY, "");                          \
    ENABLE_0(f, ComplexityPawnFlanks, EGONLY, "");                          \
    ENABLE_0(f, ComplexityPawnEndgame, EGONLY, "");                         \
    ENABLE_0(f, ComplexityAdjustment, EGONLY, "");                          \
    ENABLE_0(f, SafetyKnightWeight, SAFETY, "");                            \
    ENABLE_0(f, SafetyBishopWeight, SAFETY, "");                            \
    ENABLE_0(f, SafetyRookWeight, SAFETY, "");                              \
    ENABLE_0(f, SafetyQueenWeight, SAFETY, "");                             \
    ENABLE_0(f, SafetyAttackValue, SAFETY, "");                             \
    ENABLE_0(f, SafetyWeakSquares, SAFETY, "");                             \
    ENABLE_0(f, SafetyFriendlyPawns, SAFETY, "");                           \
    ENABLE_0(f, SafetyNoEnemyQueens, SAFETY, "");                           \
    ENABLE_0(f, SafetySafeQueenCheck, SAFETY, "");                          \
    ENABLE_0(f, SafetySafeRookCheck, SAFETY, "");                           \
    ENABLE_0(f, SafetySafeBishopCheck, SAFETY, "");                         \
    ENABLE_0(f, SafetySafeKnightCheck, SAFETY, "");                         \
    ENABLE_0(f, SafetyAdjustment, SAFETY, "");                              \
} while (0)

#endif
