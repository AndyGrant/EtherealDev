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
#define REPORTING    (     50) // How often to report progress
#define NTERMS       (    647) // Total terms in the Tuner (???)

#define LEARNING     (    0.1) // Learning rate
#define LRDROPRATE   (   1.25) // Cut LR by this each failure
#define MAXEPOCHS    (1000000) // Max number of epochs allowed
#define BATCHSIZE    (  16384) // FENs per mini-batch
#define NPOSITIONS   (5832688) // Total FENS in the book

#define STACKSIZE ((int)((double) NPOSITIONS * NTERMS / 8))

#define TunePawnValue                   (1)
#define TuneKnightValue                 (1)
#define TuneBishopValue                 (1)
#define TuneRookValue                   (1)
#define TuneQueenValue                  (1)
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
#define TuneKnightInSiberia             (1)
#define TuneKnightMobility              (1)
#define TuneBishopPair                  (1)
#define TuneBishopRammedPawns           (1)
#define TuneBishopOutpost               (1)
#define TuneBishopBehindPawn            (1)
#define TuneBishopLongDiagonal          (1)
#define TuneBishopMobility              (1)
#define TuneRookFile                    (1)
#define TuneRookOnSeventh               (1)
#define TuneRookMobility                (1)
#define TuneQueenRelativePin            (1)
#define TuneQueenMobility               (1)
#define TuneKingDefenders               (1)
#define TuneKingPawnFileProximity       (1)
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
#define TuneThreatMinorAttackedByKing   (1)
#define TuneThreatRookAttackedByKing    (1)
#define TuneThreatQueenAttackedByOne    (1)
#define TuneThreatOverloadedPieces      (1)
#define TuneThreatByPawnPush            (1)
#define TuneSpaceRestrictPiece          (1)
#define TuneSpaceRestrictEmpty          (1)
#define TuneSpaceCenterControl          (1)
#define TuneClosednessKnightAdjustment  (1)
#define TuneClosednessRookAdjustment    (1)
#define TuneComplexityTotalPawns        (1)
#define TuneComplexityPawnFlanks        (1)
#define TuneComplexityPawnEndgame       (1)
#define TuneComplexityAdjustment        (1)

enum { NORMAL, COMPLEXITY, METHOD_NB };

typedef struct TTuple {
    uint16_t index;
    int16_t wcoeff;
    int16_t bcoeff;
} TTuple;

typedef struct TEntry {
    int ntuples, seval, phase;
    int eval, complexity, scaleFactor;
    double result, pfactors[PHASE_NB];
    TTuple *tuples;
} TEntry;

typedef struct TGradientData {
    double mgeval, egeval, complexity;
} TGradientData;

typedef int TArray[NTERMS];

typedef double TVector[NTERMS][PHASE_NB];


void runTuner();
void initCurrentParameters(TVector cparams);
void initMethodManager(TArray methods);
void initCoefficients(TVector coeffs);
void initTunerEntries(TEntry *entries, Thread *thread, TArray methods);
void initTunerEntry(TEntry *entry, Board *board, TArray methods);
void initTunerTuples(TEntry *entry, TVector coeffs, TArray methods);

double computeOptimalK(TEntry *entries);
double staticEvaluationErrors(TEntry *entries, double K);
double tunedEvaluationErrors(TEntry *entries, TVector params, TArray methods, double K);
double sigmoid(double K, double E);

double linearEvaluation(TEntry *entry, TVector params, TArray methods, TGradientData *data);
void computeGradient(TEntry *entries, TVector gradient, TVector params, TArray methods, double K, int batch);
void updateSingleGradient(TEntry *entry, TVector gradient, TVector params, TArray methods, double K);

void printParameters(TVector params, TVector cparams);
void print_0(char *name, TVector params, int i, char *S);
void print_1(char *name, TVector params, int i, int A, char *S);
void print_2(char *name, TVector params, int i, int A, int B, char *S);
void print_3(char *name, TVector params, int i, int A, int B, int C, char *S);

// Initalize the Method Manger for an N dimensional array

#define INIT_METHOD_0(term, M, S) do {                          \
    methods[i++] = M;                                           \
} while (0)

#define INIT_METHOD_1(term, A, M, S) do {                       \
    for (int _a = 0; _a < A; _a++)                              \
        methods[i++] = M;                                       \
} while (0)

#define INIT_METHOD_2(term, A, B, M, S) do {                    \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_METHOD_1(term[_b], B, M, S);                       \
} while (0)

#define INIT_METHOD_3(term, A, B, C, M, S) do {                 \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_METHOD_2(term[_c], B, C, M, S);                    \
} while (0)

// Initalize Parameters of an N dimensional array

#define INIT_PARAM_0(term, M, S) do {                           \
     cparams[i  ][MG] = ScoreMG(term);                          \
     cparams[i++][EG] = ScoreEG(term);                          \
} while (0)

#define INIT_PARAM_1(term, A, M, S) do {                        \
    for (int _a = 0; _a < A; _a++)                              \
       {cparams[i  ][MG] = ScoreMG(term[_a]);                   \
        cparams[i++][EG] = ScoreEG(term[_a]);}                  \
} while (0)

#define INIT_PARAM_2(term, A, B, M, S) do {                     \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_PARAM_1(term[_b], B, M, S);                        \
} while (0)

#define INIT_PARAM_3(term, A, B, C, M, S) do {                  \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_PARAM_2(term[_c], B, C, M, S);                     \
} while (0)

// Initalize Coefficients from an N dimensional array

#define INIT_COEFF_0(term, M, S) do {                           \
    coeffs[i  ][WHITE] = T.term[WHITE];                         \
    coeffs[i++][BLACK] = T.term[BLACK];                         \
} while (0)

#define INIT_COEFF_1(term, A, M, S) do {                        \
    for (int _a = 0; _a < A; _a++)                              \
      { coeffs[i  ][WHITE] = T.term[_a][WHITE];                 \
        coeffs[i++][BLACK] = T.term[_a][BLACK]; }               \
} while (0)

#define INIT_COEFF_2(term, A, B, M, S) do {                     \
    for (int _b = 0; _b < A; _b++)                              \
        INIT_COEFF_1(term[_b], B, M, S);                        \
} while (0)

#define INIT_COEFF_3(term, A, B, C, M, S) do {                  \
    for (int _c = 0; _c < A; _c++)                              \
        INIT_COEFF_2(term[_c], B, C, M, S);                     \
} while (0)

// Print Parameters of an N dimensional array

#define PRINT_0(term, M, S) (print_0(#term, tparams, i, S), i+=1)

#define PRINT_1(term, A, M, S) (print_1(#term, tparams, i, A, S), i+=A)

#define PRINT_2(term, A, B, M, S) (print_2(#term, tparams, i, A, B, S), i+=A*B)

#define PRINT_3(term, A, B, C, M, S) (print_3(#term, tparams, i, A, B, C, S), i+=A*B*C)

// Generic wrapper for all of the above functions

#define ENABLE_0(F, term, M, S) do {                            \
    if (Tune##term) F##_0(term, M, S);                          \
} while (0)

#define ENABLE_1(F, term, A, M, S) do {                         \
    if (Tune##term) F##_1(term, A, M, S);                       \
} while (0)

#define ENABLE_2(F, term, A, B, M, S) do {                      \
    if (Tune##term) F##_2(term, A, B, M, S);                    \
} while (0)

#define ENABLE_3(F, term, A, B, C, M, S) do {                   \
    if (Tune##term) F##_3(term, A, B, C, M, S);                 \
} while (0)

// Configuration for each aspect of the evaluation terms

#define EXECUTE_ON_TERMS(F) do {                                            \
    ENABLE_0(F, PawnValue, NORMAL, "");                                     \
    ENABLE_0(F, KnightValue, NORMAL, "");                                   \
    ENABLE_0(F, BishopValue, NORMAL, "");                                   \
    ENABLE_0(F, RookValue, NORMAL, "");                                     \
    ENABLE_0(F, QueenValue, NORMAL, "");                                    \
    ENABLE_1(F, PawnPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_1(F, KnightPSQT32, 32, NORMAL, "[32]");                          \
    ENABLE_1(F, BishopPSQT32, 32, NORMAL, "[32]");                          \
    ENABLE_1(F, RookPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_1(F, QueenPSQT32, 32, NORMAL, "[32]");                           \
    ENABLE_1(F, KingPSQT32, 32, NORMAL, "[32]");                            \
    ENABLE_2(F, PawnCandidatePasser, 2, 8, NORMAL, "[2][RANK_NB]");         \
    ENABLE_0(F, PawnIsolated, NORMAL, "");                                  \
    ENABLE_1(F, PawnStacked, 2, NORMAL, "[2]");                             \
    ENABLE_2(F, PawnBackwards, 2, 8, NORMAL, "[2][RANK_NB]");               \
    ENABLE_1(F, PawnConnected32, 32, NORMAL, "[32]");                       \
    ENABLE_2(F, KnightOutpost, 2, 2, NORMAL, "[2][2]");                     \
    ENABLE_0(F, KnightBehindPawn, NORMAL, "");                              \
    ENABLE_1(F, KnightInSiberia, 4, NORMAL, "[4]");                         \
    ENABLE_1(F, KnightMobility, 9, NORMAL, "[9]");                          \
    ENABLE_0(F, BishopPair, NORMAL, "");                                    \
    ENABLE_0(F, BishopRammedPawns, NORMAL, "");                             \
    ENABLE_2(F, BishopOutpost, 2, 2, NORMAL, "[2][2]");                     \
    ENABLE_0(F, BishopBehindPawn, NORMAL, "");                              \
    ENABLE_0(F, BishopLongDiagonal, NORMAL, "");                            \
    ENABLE_1(F, BishopMobility, 14, NORMAL, "[14]");                        \
    ENABLE_1(F, RookFile, 2, NORMAL, "[2]");                                \
    ENABLE_0(F, RookOnSeventh, NORMAL, "");                                 \
    ENABLE_1(F, RookMobility, 15, NORMAL, "[15]");                          \
    ENABLE_0(F, QueenRelativePin, NORMAL, "");                              \
    ENABLE_1(F, QueenMobility, 28, NORMAL, "[28]");                         \
    ENABLE_1(F, KingDefenders, 12, NORMAL, "[12]");                         \
    ENABLE_1(F, KingPawnFileProximity, 8, NORMAL, "[FILE_NB]");             \
    ENABLE_3(F, KingShelter, 2, 8, 8, NORMAL, "[2][FILE_NB][RANK_NB]");     \
    ENABLE_3(F, KingStorm, 2, 4, 8, NORMAL, "[2][FILE_NB/2][RANK_NB]");     \
    ENABLE_3(F, PassedPawn, 2, 2, 8, NORMAL, "[2][2][RANK_NB]");            \
    ENABLE_1(F, PassedFriendlyDistance, 8, NORMAL, "[FILE_NB]");            \
    ENABLE_1(F, PassedEnemyDistance, 8, NORMAL, "[FILE_NB]");               \
    ENABLE_0(F, PassedSafePromotionPath, NORMAL, "");                       \
    ENABLE_0(F, ThreatWeakPawn, NORMAL, "");                                \
    ENABLE_0(F, ThreatMinorAttackedByPawn, NORMAL, "");                     \
    ENABLE_0(F, ThreatMinorAttackedByMinor, NORMAL, "");                    \
    ENABLE_0(F, ThreatMinorAttackedByMajor, NORMAL, "");                    \
    ENABLE_0(F, ThreatRookAttackedByLesser, NORMAL, "");                    \
    ENABLE_0(F, ThreatMinorAttackedByKing, NORMAL, "");                     \
    ENABLE_0(F, ThreatRookAttackedByKing, NORMAL, "");                      \
    ENABLE_0(F, ThreatQueenAttackedByOne, NORMAL, "");                      \
    ENABLE_0(F, ThreatOverloadedPieces, NORMAL, "");                        \
    ENABLE_0(F, ThreatByPawnPush, NORMAL, "");                              \
    ENABLE_0(F, SpaceRestrictPiece, NORMAL, "");                            \
    ENABLE_0(F, SpaceRestrictEmpty, NORMAL, "");                            \
    ENABLE_0(F, SpaceCenterControl, NORMAL, "");                            \
    ENABLE_1(F, ClosednessKnightAdjustment, 9, NORMAL, "[9]");              \
    ENABLE_1(F, ClosednessRookAdjustment, 9, NORMAL, "[9]");                \
    ENABLE_0(F, ComplexityTotalPawns, COMPLEXITY, "");                      \
    ENABLE_0(F, ComplexityPawnFlanks, COMPLEXITY, "");                      \
    ENABLE_0(F, ComplexityPawnEndgame, COMPLEXITY, "");                     \
    ENABLE_0(F, ComplexityAdjustment, COMPLEXITY, "");                      \
} while (0)

#endif