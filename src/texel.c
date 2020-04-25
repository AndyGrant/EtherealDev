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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitboards.h"
#include "board.h"
#include "evaluate.h"
#include "history.h"
#include "move.h"
#include "search.h"
#include "texel.h"
#include "thread.h"
#include "transposition.h"
#include "types.h"
#include "uci.h"
#include "zobrist.h"

// Internal Memory Managment
TTuple* TupleStack;
int TupleStackSize = STACKSIZE;

// Tap into evaluate()
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
extern const int PawnCandidatePasser[2][8];
extern const int PawnIsolated;
extern const int PawnStacked[2];
extern const int PawnBackwards[2][8];
extern const int PawnConnected32[32];
extern const int KnightOutpost[2][2];
extern const int KnightBehindPawn;
extern const int KnightInSiberia[4];
extern const int KnightMobility[9];
extern const int BishopPair;
extern const int BishopRammedPawns;
extern const int BishopOutpost[2][2];
extern const int BishopBehindPawn;
extern const int BishopLongDiagonal;
extern const int BishopMobility[14];
extern const int RookFile[2];
extern const int RookOnSeventh;
extern const int RookMobility[15];
extern const int QueenMobility[28];
extern const int KingDefenders[12];
extern const int KingPawnFileProximity[8];
extern const int KingShelter[2][8][8];
extern const int KingStorm[2][4][8];
extern const int PassedPawn[2][2][8];
extern const int PassedFriendlyDistance[8];
extern const int PassedEnemyDistance[8];
extern const int PassedSafePromotionPath;
extern const int ThreatWeakPawn;
extern const int ThreatMinorAttackedByPawn;
extern const int ThreatMinorAttackedByMinor;
extern const int ThreatMinorAttackedByMajor;
extern const int ThreatRookAttackedByLesser;
extern const int ThreatMinorAttackedByKing;
extern const int ThreatRookAttackedByKing;
extern const int ThreatQueenAttackedByOne;
extern const int ThreatOverloadedPieces;
extern const int ThreatByPawnPush;
extern const int SpaceRestrictPiece;
extern const int SpaceRestrictEmpty;
extern const int SpaceCenterControl;
extern const int ClosednessKnightAdjustment[9];
extern const int ClosednessRookAdjustment[9];
extern const int ComplexityTotalPawns;
extern const int ComplexityPawnFlanks;
extern const int ComplexityPawnEndgame;
extern const int ComplexityAdjustment;
extern const int SafetyKnightWeight;
extern const int SafetyBishopWeight;
extern const int SafetyRookWeight;
extern const int SafetyQueenWeight;
extern const int SafetyAttackValue;
extern const int SafetyWeakSquares;
extern const int SafetyFriendlyPawns;
extern const int SafetyNoEnemyQueens;
extern const int SafetySafeQueenCheck;
extern const int SafetySafeRookCheck;
extern const int SafetySafeBishopCheck;
extern const int SafetySafeKnightCheck;
extern const int SafetyAdjustment;

void runTuner(Thread *thread) {

    int iteration = -1;
    double K, error;

    TArray types;
    TEntry *entries;
    TVector params = {0};
    TVector cparams = {0};

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\nTuner Will Be Tuning %d Terms...", NTERMS);

    printf("\n\nSetting Table Size to 1MB for Speed...");
    initTT(1);

    printf("\n\nAllocating Memory for Tuner Entries [%dMB]...",
          (int)(NPOSITIONS * sizeof(TEntry) / (1024 * 1024)));
    entries = calloc(NPOSITIONS, sizeof(TEntry));

    printf("\n\nAllocating Memory for Tuple Stack [%dMB]...",
          (int)(STACKSIZE * sizeof(TTuple) / (1024 * 1024)));
    TupleStack = calloc(STACKSIZE, sizeof(TTuple));

    printf("\n\nInitializing Tuner Entries from FENS...");
    initTunerEntries(entries, thread);

    printf("\n\nFetching Current Params as a Starting Point...");
    initCurrentParameters(cparams);

    printf("\n\nSetting Term Types {NORMAL, MGONLY, EGONLY, SAFETY}...");
    initTypeManager(types);

    printf("\n\nComputing OptimaL K Value...\n");
    K = computeOptimalK(entries);

    while (1) {

        if (NPOSITIONS != BATCHSIZE)
            shuffleTunerEntries(entries);

        if (++iteration % REPORTING == 0) {

            error = completeLinearError(entries, params, cparams, types, K);
            printParameters(params, cparams);
            printf("\nIteration [%d] Error = %g \n", iteration, error);
        }

        for (int batch = 0; batch < NPOSITIONS / BATCHSIZE; batch++) {

            TVector gradient = {0};
            updateGradient(entries, gradient, params, cparams, types, K, batch);

            for (int i = 0; i < NTERMS; i++)
                for (int j = MG; j <= EG; j++)
                    params[i][j] -= (1.0 / BATCHSIZE) * LEARNING * gradient[i][j];
        }
    }
}

void initTunerEntries(TEntry *tes, Thread *thread) {

    Undo undo[1];
    Limits limits;
    char line[128];
    int i, j, k, searchEval, coeffs[NTERMS];
    FILE *fin = fopen("FENS", "r");

    // Initialize the thread for the search
    thread->limits = &limits; thread->depth  = 0;

    // Create a TEntry for each FEN
    for (i = 0; i < NPOSITIONS; i++) {

        // Read next position from the FEN file
        if (fgets(line, 128, fin) == NULL) {
            printf("Unable to read line #%d\n", i);
            exit(EXIT_FAILURE);
        }

        // Occasional reporting for total completion
        if ((i + 1) % 10000 == 0 || i == NPOSITIONS - 1)
            printf("\rINITIALIZING TUNER ENTRIES FROM FENS...  [%7d OF %7d]", i + 1, NPOSITIONS);

        // Fetch and cap a white POV search
        searchEval = atoi(strstr(line, "] ") + 2);
        if (strstr(line, " b ")) searchEval *= -1;

        // Determine the result of the game
        if      (strstr(line, "[1.0]")) tes[i].result = 1.0;
        else if (strstr(line, "[0.0]")) tes[i].result = 0.0;
        else if (strstr(line, "[0.5]")) tes[i].result = 0.5;
        else    {printf("Cannot Parse %s\n", line); exit(EXIT_FAILURE);}

        // Resolve FEN to a quiet position
        boardFromFEN(&thread->board, line, 0);
        qsearch(thread, &thread->pv, -MATE, MATE, 0);
        for (j = 0; j < thread->pv.length; j++)
            applyMove(&thread->board, thread->pv.line[j], undo);

        // Determine the game phase based on remaining material
        tes[i].phase = 24 - 4 * popcount(thread->board.pieces[QUEEN ])
                          - 2 * popcount(thread->board.pieces[ROOK  ])
                          - 1 * popcount(thread->board.pieces[BISHOP])
                          - 1 * popcount(thread->board.pieces[KNIGHT]);

        // Compute phase factors for updating the gradients and
        tes[i].factors[MG] = 1 - tes[i].phase / 24.0;
        tes[i].factors[EG] = 0 + tes[i].phase / 24.0;

        // Finish the phase calculation for the evaluation
        tes[i].phase = (tes[i].phase * 256 + 12) / 24.0;

        // Vectorize the evaluation coefficients and save the eval
        // relative to WHITE. We must first clear the coeff vector.
        T = EmptyTrace;
        tes[i].eval = evaluateBoard(&thread->board, NULL, 0);
        if (thread->board.turn == BLACK) tes[i].eval *= -1;
        initCoefficients(coeffs);

        // Weight the Static and Search evals
        tes[i].eval = tes[i].eval * STATICWEIGHT
                    +  searchEval * SEARCHWEIGHT;

        // Count up the non zero coefficients
        for (k = 0, j = 0; j < NTERMS; j++)
            k += coeffs[j] != 0;

        // Allocate Tuples
        updateMemory(&tes[i], k);

        // Initialize the Tuner Tuples
        for (k = 0, j = 0; j < NTERMS; j++) {
            if (coeffs[j] != 0){
                tes[i].tuples[k].index = j;
                tes[i].tuples[k++].coeff = coeffs[j];
            }
        }
    }

    fclose(fin);
}


void initTypeManager(TArray types) {

    int i = 0; // EXECUTE_ON_TERMS() will update i accordingly

    EXECUTE_ON_TERMS(INIT_TYPE);

    if (i != NTERMS){
        printf("Error in initTypeManager(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void initCoefficients(TArray coeffs) {

    int i = 0; // EXECUTE_ON_TERMS() will update i accordingly

    EXECUTE_ON_TERMS(INIT_COEFF);

    if (i != NTERMS){
        printf("Error in initCoefficients(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void initCurrentParameters(TVector cparams) {

    int i = 0; // EXECUTE_ON_TERMS() will update i accordingly

    EXECUTE_ON_TERMS(INIT_PARAM);

    if (i != NTERMS){
        printf("Error in initCurrentParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}


void updateMemory(TEntry *te, int size) {

    // First ensure we have enough Tuples left for this TEntry
    if (size > TupleStackSize) {
        printf("\n\nALLOCATING MEMORY FOR TUNER TUPLE STACK [%dMB]...\n\n",
                (int)(STACKSIZE * sizeof(TTuple) / (1024 * 1024)));
        TupleStackSize = STACKSIZE;
        TupleStack = calloc(STACKSIZE, sizeof(TTuple));
    }

    // Allocate Tuples for the given TEntry
    te->tuples = TupleStack;
    te->ntuples = size;

    // Update internal memory manager
    TupleStack += size;
    TupleStackSize -= size;
}

void updateGradient(TEntry *entries, TVector gradient, TVector params, TVector cparams, TArray types, double K, int batch) {

    #pragma omp parallel shared(gradient)
    {
        TVector local = {0};
        #pragma omp for schedule(static, BATCHSIZE / NPARTITIONS)
        for (int i = batch * BATCHSIZE; i < (batch + 1) * BATCHSIZE; i++) {

            double R = entries[i].result;
            double S = sigmoid(K, linearEvaluation(&entries[i], params, cparams, types));
            double D = -(R - S) * S * (1.0 - S);

            for (int j = 0; j < entries[i].ntuples; j++) {
                int index = entries[i].tuples[j].index;
                int coeff = entries[i].tuples[j].coeff;
                local[index][MG] += D * computeGradient(&entries[i], index, coeff, types, MG);
                local[index][EG] += D * computeGradient(&entries[i], index, coeff, types, EG);
            }
        }

        for (int i = 0; i < NTERMS; i++) {
            if (types[i] != EGONLY) gradient[i][MG] += local[i][MG];
            if (types[i] != MGONLY) gradient[i][EG] += local[i][EG];
        }
    }
}

double computeGradient(TEntry *entry, int index, int coeff, TArray types, int phase) {

    if (types[index] != SAFETY)
        return entry->factors[phase] * coeff;

    return entry->factors[phase] * (BlackCoeff(coeff) - WhiteCoeff(coeff)) / 20.0;
}

void shuffleTunerEntries(TEntry *tes) {

    for (int i = 0; i < NPOSITIONS; i++) {

        int A = rand64() % NPOSITIONS;
        int B = rand64() % NPOSITIONS;

        TEntry temp = tes[A];
        tes[A] = tes[B];
        tes[B] = temp;
    }
}


double sigmoid(double K, double S) {
    return 1.0 / (1.0 + exp(-K * S / 400.0));
}

double computeOptimalK(TEntry *tes) {

    double start = -10.0, end = 10.0, delta = 1.0;
    double curr = start, error, best = completeEvaluationError(tes, start);

    for (int i = 0; i < KPRECISION; i++) {

        curr = start - delta;
        while (curr < end) {
            curr = curr + delta;
            error = completeEvaluationError(tes, curr);
            if (error <= best)
                best = error, start = curr;
        }

        printf("COMPUTING K ITERATION [%d] K = %f E = %f\n", i, start, best);

        end = start + delta;
        start = start - delta;
        delta = delta / 10.0;
    }

    return start;
}

double completeEvaluationError(TEntry *tes, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(tes[i].result - sigmoid(K, tes[i].eval), 2);
    }

    return total / (double)NPOSITIONS;
}

double completeLinearError(TEntry *tes, TVector params, TVector cparams, TArray types, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(tes[i].result - sigmoid(K, linearEvaluation(&tes[i], params, cparams, types)), 2);
    }

    return total / (double)NPOSITIONS;
}


double linearEvaluation(TEntry *entry, TVector params, TVector cparams, TArray types) {

    return linearEvaluationNormal(entry, params, types) + entry->eval
         + linearEvaluationSafety(entry, params, cparams, types);
}

double linearEvaluationNormal(TEntry *entry, TVector params, TArray types) {

    double mg = 0, eg = 0;

    // Recompute evaluation for NORMAL terms
    for (int i = 0; i < entry->ntuples; i++) {
        if (types[entry->tuples[i].index] != SAFETY) {
            mg += entry->tuples[i].coeff * params[entry->tuples[i].index][MG];
            eg += entry->tuples[i].coeff * params[entry->tuples[i].index][EG];
        }
    }

    // Interpolate the evaluation by game phase
    return ((mg * (256 - entry->phase) + eg * entry->phase) / 256.0);
}

double linearEvaluationSafety(TEntry *entry, TVector params, TVector cparams, TArray types) {

    int index, wcoeff, bcoeff;

    double mg, eg;
    double safety[COLOUR_NB][PHASE_NB] = {0};
    double original[COLOUR_NB][PHASE_NB] = {0};

    for (int i = 0; i < entry->ntuples; i++) {

        // Only deal with the SAFETY terms
        if (types[entry->tuples[i].index] != SAFETY) continue;

        // Coeffs are packed by colour
        index = entry->tuples[i].index;
        wcoeff = WhiteCoeff(entry->tuples[i].coeff);
        bcoeff = BlackCoeff(entry->tuples[i].coeff);

        // Rebuild the original evaluation (pre-formula)
        original[WHITE][MG] += wcoeff * cparams[index][MG];
        original[WHITE][EG] += wcoeff * cparams[index][EG];
        original[BLACK][MG] += bcoeff * cparams[index][MG];
        original[BLACK][EG] += bcoeff * cparams[index][EG];

        // Build the updated evaluation (pre-formula)
        safety[WHITE][MG] += wcoeff * (cparams[index][MG] + params[index][MG]);
        safety[WHITE][EG] += wcoeff * (cparams[index][EG] + params[index][EG]);
        safety[BLACK][MG] += bcoeff * (cparams[index][MG] + params[index][MG]);
        safety[BLACK][EG] += bcoeff * (cparams[index][EG] + params[index][EG]);
    }

    // EG = -OldWhite + OldBlack + NewWhite - NewBlack
    mg = -(-original[WHITE][MG] / 20) + -(-original[BLACK][MG] / 20)
       +  (-  safety[WHITE][MG] / 20) -  (-  safety[BLACK][MG] / 20);

    // EG = -OldWhite + OldBlack + NewWhite - NewBlack
    eg = -(-original[WHITE][EG] / 20) + -(-original[BLACK][EG] / 20)
       +  (-  safety[WHITE][EG] / 20) -  (-  safety[BLACK][EG] / 20);

    // Interpolate the evaluation by game phase
    return ((mg * (256 - entry->phase) + eg * entry->phase) / 256.0);
}

void printParameters(TVector params, TVector cparams) {

    int tparams[NTERMS][PHASE_NB];

    // Combine updated and current parameters
    for (int j = 0; j < NTERMS; j++) {
        tparams[j][MG] = round(params[j][MG] + cparams[j][MG]);
        tparams[j][EG] = round(params[j][EG] + cparams[j][EG]);
    }

    int i = 0; // EXECUTE_ON_TERMS will update i accordingly

    EXECUTE_ON_TERMS(PRINT);

    if (i != NTERMS) {
        printf("Error in printParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void print_0(char *name, int params[NTERMS][PHASE_NB], int i, char *S) {

    printf("const int %s%s = S(%4d,%4d);\n\n", name, S, params[i][MG], params[i][EG]);

}

void print_1(char *name, int params[NTERMS][PHASE_NB], int i, int A, char *S) {

    printf("const int %s%s = { ", name, S);

    if (A >= 3) {

        for (int a = 0; a < A; a++, i++) {
            if (a % 4 == 0) printf("\n    ");
            printf("S(%4d,%4d), ", params[i][MG], params[i][EG]);
        }

        printf("\n};\n\n");
    }

    else {

        for (int a = 0; a < A; a++, i++) {
            printf("S(%4d,%4d)", params[i][MG], params[i][EG]);
            if (a != A - 1) printf(", "); else printf(" };\n\n");
        }
    }

}

void print_2(char *name, int params[NTERMS][PHASE_NB], int i, int A, int B, char *S) {

    printf("const int %s%s = {\n", name, S);

    for (int a = 0; a < A; a++) {

        printf("   {");

        for (int b = 0; b < B; b++, i++) {
            if (b && b % 4 == 0) printf("\n    ");
            printf("S(%4d,%4d)", params[i][MG], params[i][EG]);
            printf("%s", b == B - 1 ? "" : ", ");
        }

        printf("},\n");
    }

    printf("};\n\n");

}

void print_3(char *name, int params[NTERMS][PHASE_NB], int i, int A, int B, int C, char *S) {

    printf("const int %s%s = {\n", name, S);

    for (int a = 0; a < A; a++) {

        for (int b = 0; b < B; b++) {

            printf("%s", b ? "   {" : "  {{");;

            for (int c = 0; c < C; c++, i++) {
                if (c &&  c % 4 == 0) printf("\n    ");
                printf("S(%4d,%4d)", params[i][MG], params[i][EG]);
                printf("%s", c == C - 1 ? "" : ", ");
            }

            printf("%s", b == B - 1 ? "}},\n" : "},\n");
        }

    }

    printf("};\n\n");
}

#endif
