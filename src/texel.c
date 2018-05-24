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

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "evaluate.h"
#include "move.h"
#include "piece.h"
#include "psqt.h"
#include "texel.h"
#include "thread.h"
#include "types.h"
#include "uci.h"

extern EvalTrace T, EmptyTrace;

extern const int PawnValue;
extern const int KnightValue;
extern const int BishopValue;
extern const int RookValue;
extern const int QueenValue;
extern const int KingValue;
extern const int PawnPSQT32[32];
extern const int KnightPSQT32[32];
extern const int BishopPSQT32[32];
extern const int RookPSQT32[32];
extern const int QueenPSQT32[32];
extern const int KingPSQT32[32];
extern const int PawnIsolated;
extern const int PawnStacked;
extern const int PawnBackwards[2];
extern const int PawnConnected32[32];
extern const int KnightRammedPawns;
extern const int KnightOutpost[2];
extern const int KnightMobility[9];
extern const int BishopPair;
extern const int BishopRammedPawns;
extern const int BishopOutpost[2];
extern const int BishopMobility[14];
extern const int RookFile[2];
extern const int RookOnSeventh;
extern const int RookMobility[15];
extern const int QueenMobility[28];
extern const int KingDefenders[12];
extern const int KingShelter[2][8][8];
extern const int PassedPawn[2][2][8];
extern const int ThreatPawnAttackedByOne;
extern const int ThreatMinorAttackedByPawn;
extern const int ThreatMinorAttackedByMajor;
extern const int ThreatQueenAttackedByOne;
extern const int Tempo;
extern const int KingSafetyBaseLine;
extern const int KingSafetyThreatWeight[PIECE_NB];
extern const int KingSafetyWeakSquares;
extern const int KingSafetyFriendlyPawns;
extern const int KingSafetyNoEnemyQueens;

void runTexelTuning(Thread *thread) {

    TexelEntry *tes;
    int enabled[NTERMS] = {0};
    int params[NTERMS][PHASE_NB] = {{0}, {0}};
    double K, bestError, thisError;

    setvbuf(stdout, NULL, _IONBF, 0); // Auto flush

    printf("\nEvaluation contains %d Terms...", NTERMS);

    printf("\n\nAllocating Memory for Texel Entries [%dMB]...",
           (int)(NPOSITONS * sizeof(TexelEntry) / (1024 * 1024)));
    tes = calloc(NPOSITONS, sizeof(TexelEntry));

    printf("\n\nSetting Flags for Enabled Tuning Terms...");
    printf("\n\nTuner will be Tuning %d Terms...", initEnabledTerms(enabled));

    printf("\n\nFetching Current Evaluation Terms as a Starting Point...");
    initParameters(params);

    printf("\n\nReading and Initializing Texel Entries from FENS...");
    initTexelEntries(tes, params, thread);

    printf("\n\nComputing Optimal K Value...\n");
    K = computeOptimalK(tes);

    bestError = completeEvaluationError(tes, K);

    for (int iteration = 0; ; iteration++) {

        int improved = 0;

        printf("ITERATION [%3d] Error=%f", iteration, bestError);
        for (int i = 0; i < NTERMS; i++)
            if (enabled[i])
                printf("{%4d %4d} ", params[i][MG], params[i][EG]);
        printf("\n");

        for (int p = 0; p < NTERMS * 2; p++) {

            if (!enabled[p/2])
                continue;

            int *param = &params[p/2][p%2];

            *param += 1;
            thisError = evaluationError(tes, params, K);
            if (thisError + DBL_EPSILON < bestError) {
                bestError = thisError;
                improved = 1;
                continue;
            }

            *param -= 2;
            thisError = evaluationError(tes, params, K);
            if (thisError + DBL_EPSILON < bestError) {
                bestError = thisError;
                improved = 1;
                continue;
            }

            *param += 1;
        }

        if (!improved)
            break;
    }

}

int initEnabledTerms(int enabled[NTERMS]) {

    int i = 0, terms = 0;

    SET_ENABLED(PawnValue, 1);
    SET_ENABLED(KnightValue, 1);
    SET_ENABLED(BishopValue, 1);
    SET_ENABLED(RookValue, 1);
    SET_ENABLED(QueenValue, 1);
    SET_ENABLED(KingValue, 1);
    SET_ENABLED(PawnPSQT32, 32);
    SET_ENABLED(KnightPSQT32, 32);
    SET_ENABLED(BishopPSQT32, 32);
    SET_ENABLED(RookPSQT32, 32);
    SET_ENABLED(QueenPSQT32, 32);
    SET_ENABLED(KingPSQT32, 32);
    SET_ENABLED(PawnIsolated, 1);
    SET_ENABLED(PawnStacked, 1);
    SET_ENABLED(PawnBackwards, 2);
    SET_ENABLED(PawnConnected32, 32);
    SET_ENABLED(KnightRammedPawns, 1);
    SET_ENABLED(KnightOutpost, 2);
    SET_ENABLED(KnightMobility, 9);
    SET_ENABLED(BishopPair, 1);
    SET_ENABLED(BishopRammedPawns, 1);
    SET_ENABLED(BishopOutpost, 2);
    SET_ENABLED(BishopMobility, 14);
    SET_ENABLED(RookFile, 2);
    SET_ENABLED(RookOnSeventh, 1);
    SET_ENABLED(RookMobility, 15);
    SET_ENABLED(QueenMobility, 28);
    SET_ENABLED(KingDefenders, 12);
    SET_ENABLED(KingShelter, 128);
    SET_ENABLED(PassedPawn, 32);
    SET_ENABLED(ThreatPawnAttackedByOne, 1);
    SET_ENABLED(ThreatMinorAttackedByPawn, 1);
    SET_ENABLED(ThreatMinorAttackedByMajor, 1);
    SET_ENABLED(ThreatQueenAttackedByOne, 1);
    SET_ENABLED(Tempo, 1);
    SET_ENABLED(KingSafetyBaseLine, 1);
    SET_ENABLED(KingSafetyThreatWeight, 6);
    SET_ENABLED(KingSafetyWeakSquares, 1);
    SET_ENABLED(KingSafetyFriendlyPawns, 1);
    SET_ENABLED(KingSafetyNoEnemyQueens, 1);

    assert(i == NTERMS); // Set every value

    return terms; // Total set terms
}

void initParameters(int params[NTERMS][PHASE_NB]) {

    int i = 0;

    INIT_PARAM_0(PawnValue, 1);
    INIT_PARAM_0(KnightValue, 1);
    INIT_PARAM_0(BishopValue, 1);
    INIT_PARAM_0(RookValue, 1);
    INIT_PARAM_0(QueenValue, 1);
    INIT_PARAM_0(KingValue, 1);
    INIT_PARAM_1(PawnPSQT32, 32);
    INIT_PARAM_1(KnightPSQT32, 32);
    INIT_PARAM_1(BishopPSQT32, 32);
    INIT_PARAM_1(RookPSQT32, 32);
    INIT_PARAM_1(QueenPSQT32, 32);
    INIT_PARAM_1(KingPSQT32, 32);
    INIT_PARAM_0(PawnIsolated, 1);
    INIT_PARAM_0(PawnStacked, 1);
    INIT_PARAM_1(PawnBackwards, 2);
    INIT_PARAM_1(PawnConnected32, 32);
    INIT_PARAM_0(KnightRammedPawns, 1);
    INIT_PARAM_1(KnightOutpost, 2);
    INIT_PARAM_1(KnightMobility, 9);
    INIT_PARAM_0(BishopPair, 1);
    INIT_PARAM_0(BishopRammedPawns, 1);
    INIT_PARAM_1(BishopOutpost, 2);
    INIT_PARAM_1(BishopMobility, 14);
    INIT_PARAM_1(RookFile, 2);
    INIT_PARAM_0(RookOnSeventh, 1);
    INIT_PARAM_1(RookMobility, 15);
    INIT_PARAM_1(QueenMobility, 28);
    INIT_PARAM_1(KingDefenders, 12);
    INIT_PARAM_3(KingShelter, 2, 8, 8);
    INIT_PARAM_3(PassedPawn, 2, 2, 8);
    INIT_PARAM_0(ThreatPawnAttackedByOne, 1);
    INIT_PARAM_0(ThreatMinorAttackedByPawn, 1);
    INIT_PARAM_0(ThreatMinorAttackedByMajor, 1);
    INIT_PARAM_0(ThreatQueenAttackedByOne, 1);
    INIT_PARAM_0(Tempo, 1);
    INIT_PARAM_0(KingSafetyBaseLine, 1);
    INIT_PARAM_1(KingSafetyThreatWeight, 6);
    INIT_PARAM_0(KingSafetyWeakSquares, 1);
    INIT_PARAM_0(KingSafetyFriendlyPawns, 1);
    INIT_PARAM_0(KingSafetyNoEnemyQueens, 1);

    assert(i == NTERMS); // Set every value
}

void initTexelEntries(TexelEntry *tes, int params[NTERMS][PHASE_NB], Thread *thread) {

    Undo undo[1];
    char line[128];

    // Initialize limits for the search
    Limits limits;
    limits.limitedByNone  = 0;
    limits.limitedByTime  = 0;
    limits.limitedByDepth = 1;
    limits.limitedBySelf  = 0;
    limits.timeLimit      = 0;
    limits.depthLimit     = 1;

    // Initialize the thread for the search
    thread->limits = &limits;
    thread->depth  = 1;

    FILE *fin = fopen("FENS", "r");

    for (int i = 0; i < NPOSITONS; i++){

        if ((i + 1) % 50000 == 0 || i == NPOSITONS - 1)
            printf("\nInitializing Texel Entries ...  [%7d of %7d]", i + 1, NPOSITONS);

        fgets(line, 128, fin);

        // Determine the result of the game
        if      (strstr(line, "1-0")) tes[i].result = 1.0;
        else if (strstr(line, "0-1")) tes[i].result = 0.0;
        else if (strstr(line, "1/2")) tes[i].result = 0.5;
        else    {printf("Unable to Parse Result: %s\n", line); exit(0);}

        // Push through PV to a quiet position
        boardFromFEN(&thread->board, line);
        qsearch(thread, &thread->pv, -MATE, MATE, 0);
        for (int j = 0; j < thread->pv.length; j++)
            applyMove(&thread->board, thread->pv.line[j], undo);

        // Get an eval trace from this position, and an eval
        // from white's perspective, which must be done after
        // calling evaluateBoard() to preserve Tempo values
        T = EmptyTrace;
        tes[i].eval = evaluateBoard(&thread->board, NULL);
        if (thread->board.turn == BLACK) tes[i].eval = -tes[i].eval;

        // Compute phase just once to save on popcount calls
        tes[i].phase = 24 - 4 * popcount(thread->board.pieces[QUEEN ])
                          - 2 * popcount(thread->board.pieces[ROOK  ])
                          - 1 * popcount(thread->board.pieces[KNIGHT])
                          - 1 * popcount(thread->board.pieces[BISHOP]);
        tes[i].phase = (tes[i].phase * 256 + 12) / 24;

        // Store off all of the coefficients
        initTexelEntry(&tes[i]);

        // evaluateBoard() must match our linearEvaluation()
        if (tes[i].eval != linearEvaluation(&tes[i], params)){
            printf(" ERROR #%d: Linear Evaluation != Actual Evaluation !!! \n", i);
            printf(" Actual = %d Linear = %d\n", tes[i].eval, linearEvaluation(&tes[i], params));
            exit(EXIT_FAILURE);
        }
    }

    fclose(fin);
}

void initTexelEntry(TexelEntry *te) {

    int i = 0;

    INIT_NORMAL_COEFF_0(PawnValue, 1);
    INIT_NORMAL_COEFF_0(KnightValue, 1);
    INIT_NORMAL_COEFF_0(BishopValue, 1);
    INIT_NORMAL_COEFF_0(RookValue, 1);
    INIT_NORMAL_COEFF_0(QueenValue, 1);
    INIT_NORMAL_COEFF_0(KingValue, 1);
    INIT_NORMAL_COEFF_1(PawnPSQT32, 32);
    INIT_NORMAL_COEFF_1(KnightPSQT32, 32);
    INIT_NORMAL_COEFF_1(BishopPSQT32, 32);
    INIT_NORMAL_COEFF_1(RookPSQT32, 32);
    INIT_NORMAL_COEFF_1(QueenPSQT32, 32);
    INIT_NORMAL_COEFF_1(KingPSQT32, 32);
    INIT_NORMAL_COEFF_0(PawnIsolated, 1);
    INIT_NORMAL_COEFF_0(PawnStacked, 1);
    INIT_NORMAL_COEFF_1(PawnBackwards, 2);
    INIT_NORMAL_COEFF_1(PawnConnected32, 32);
    INIT_NORMAL_COEFF_0(KnightRammedPawns, 1);
    INIT_NORMAL_COEFF_1(KnightOutpost, 2);
    INIT_NORMAL_COEFF_1(KnightMobility, 9);
    INIT_NORMAL_COEFF_0(BishopPair, 1);
    INIT_NORMAL_COEFF_0(BishopRammedPawns, 1);
    INIT_NORMAL_COEFF_1(BishopOutpost, 2);
    INIT_NORMAL_COEFF_1(BishopMobility, 14);
    INIT_NORMAL_COEFF_1(RookFile, 2);
    INIT_NORMAL_COEFF_0(RookOnSeventh, 1);
    INIT_NORMAL_COEFF_1(RookMobility, 15);
    INIT_NORMAL_COEFF_1(QueenMobility, 28);
    INIT_NORMAL_COEFF_1(KingDefenders, 12);
    INIT_NORMAL_COEFF_3(KingShelter, 2, 8, 8);
    INIT_NORMAL_COEFF_3(PassedPawn, 2, 2, 8);
    INIT_NORMAL_COEFF_0(ThreatPawnAttackedByOne, 1);
    INIT_NORMAL_COEFF_0(ThreatMinorAttackedByPawn, 1);
    INIT_NORMAL_COEFF_0(ThreatMinorAttackedByMajor, 1);
    INIT_NORMAL_COEFF_0(ThreatQueenAttackedByOne, 1);
    INIT_NORMAL_COEFF_0(Tempo, 1);
    INIT_KS_COEFF_0(KingSafetyBaseLine, 1);
    INIT_KS_COEFF_1(KingSafetyThreatWeight, 6);
    INIT_KS_COEFF_0(KingSafetyWeakSquares, 1);
    INIT_KS_COEFF_0(KingSafetyFriendlyPawns, 1);
    INIT_KS_COEFF_0(KingSafetyNoEnemyQueens, 1);

    te->kingSafety[WHITE] = T.KingSafety[WHITE];
    te->kingSafety[BLACK] = T.KingSafety[BLACK];

    assert(i == NTERMS); // Set every value

}

double computeOptimalK(TexelEntry *tes) {

    int i;
    double start = -10.0, end = 10.0, delta = 1.0;
    double curr = start, thisError, bestError = completeEvaluationError(tes, start);

    for (i = 0; i < 10; i++){
        printf("Computing K Iteration [%d] ", i);

        // Find the best value if K within the range [start, end],
        // with a step size based on the current iteration
        curr = start - delta;
        while (curr < end){
            curr = curr + delta;
            thisError = completeEvaluationError(tes, curr);
            if (thisError <= bestError)
                bestError = thisError, start = curr;
        }

        printf("K = %f E = %f\n", start, bestError);

        // Narrow our search to [best - delta, best + delta]
        end = start + delta;
        start = start - delta;
        delta = delta / 10.0;
    }

    return start;
}

double completeEvaluationError(TexelEntry *tes, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITONS / NCORES) reduction(+:total)
        for (int i = 0; i < NPOSITONS; i++)
            total += pow(tes[i].result - sigmoid(K, tes[i].eval), 2);
    }

    return total / (double) NPOSITONS;
}

double evaluationError(TexelEntry *tes, int params[NTERMS][PHASE_NB], double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITONS / NCORES) reduction(+:total)
        for (int i = 0; i < NPOSITONS; i++)
            total += pow(tes[i].result - sigmoid(K, linearEvaluation(&tes[i], params)), 2);
    }

    return total / (double) NPOSITONS;
}

int linearEvaluation(TexelEntry *te, int params[NTERMS][PHASE_NB]) {

    int mgCount, egCount;
    int mg = 0, eg = 0;

    for (int i = 0; i < NNTERMS; i++){
        int coeff = te->evalCoeffs[i];
        mg += coeff * params[i][MG];
        eg += coeff * params[i][EG];
    }

    for (int c = WHITE; c <= BLACK; c++){

        if (!te->kingSafety[c])
            continue;

        mgCount = egCount = 0;
        for (int i = NNTERMS; i < NTERMS; i++){
            mgCount += te->ksCoeffs[i-NNTERMS][c] * params[i][MG];
            egCount += te->ksCoeffs[i-NNTERMS][c] * params[i][EG];
        }

        mgCount = -MAX(0, mgCount) * MAX(0, mgCount) /  64;
        egCount = -MAX(0, egCount);

        mg += c == WHITE ? mgCount : -mgCount;
        eg += c == WHITE ? egCount : -egCount;
    }

    return (mg * (256 - te->phase) + eg * te->phase) / 256;
}

double sigmoid(double K, double eval){
    return 1.0 / (1.0 + pow(10.0, -K * eval / 400.0));
}


#endif