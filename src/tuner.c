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
#include "tuner.h"
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
extern const int QueenRelativePin;
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


void runTuner() {

    TEntry *entries;
    TArray methods = {0};
    TVector params = {0}, cparams = {0};
    Thread *thread = createThreadPool(1);
    double K, error, best = 1e6, rate = LEARNING;

    const int tentryMB = (int)(NPOSITIONS * sizeof(TEntry) / (1 << 20));
    const int ttupleMB = (int)(STACKSIZE  * sizeof(TTuple) / (1 << 20));

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Tuner will be tuning %d Terms\n", NTERMS);
    printf("Allocating Memory for Tuner Entries [%dMB]\n", tentryMB);
    printf("Allocating Memory for Tuner Tuple Stack [%dMB]\n", ttupleMB);
    printf("Saving the current value for each Term as a starting point\n");
    printf("Marking each Term based on method { NORMAL, SAFETY, COMPLEXITY }\n\n");

    entries    = calloc(NPOSITIONS, sizeof(TEntry));
    TupleStack = calloc(STACKSIZE , sizeof(TTuple));

    initCurrentParameters(cparams);
    initMethodManager(methods);
    initTunerEntries(entries, thread, methods);
    K = computeOptimalK(entries);

    for (int epoch = 0; epoch < MAXEPOCHS; epoch++) {

        if (epoch % REPORTING == 0) {

            error = tunedEvaluationErrors(entries, params, methods, K);
            if (error > best) rate = rate / LRDROPRATE;
            best = error;

            printParameters(params, cparams);
            printf("\nEpoch [%d] Error = [%g] Learning = [%g]\n", epoch, error, rate);
        }

        for (int batch = 0; batch < NPOSITIONS / BATCHSIZE; batch++) {

            TVector gradient = {0};
            computeGradient(entries, gradient, params, methods, K, batch);

            for (int i = 0; i < NTERMS; i++) {
                params[i][MG] += (2.0 / BATCHSIZE) * rate * gradient[i][MG];
                params[i][EG] += (2.0 / BATCHSIZE) * rate * gradient[i][EG];
            }
        }
    }
}

void initCurrentParameters(TVector cparams) {

    int i = 0; // EXECUTE_ON_TERMS will update i accordingly

    EXECUTE_ON_TERMS(INIT_PARAM);

    if (i != NTERMS){
        printf("Error in initCurrentParameters(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void initMethodManager(TArray methods) {

    int i = 0; // EXECUTE_ON_TERMS will update i accordingly

    EXECUTE_ON_TERMS(INIT_METHOD);

    if (i != NTERMS){
        printf("Error in initMethodManager(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void initCoefficients(TVector coeffs) {

    int i = 0; // EXECUTE_ON_TERMS will update i accordingly

    EXECUTE_ON_TERMS(INIT_COEFF);

    if (i != NTERMS){
        printf("Error in initCoefficients(): i = %d ; NTERMS = %d\n", i, NTERMS);
        exit(EXIT_FAILURE);
    }
}

void initTunerEntries(TEntry *entries, Thread *thread, TArray methods) {

    Undo undo;
    char line[256];
    Limits limits = {0};
    thread->limits = &limits; thread->depth  = 0;

    FILE *fin = fopen("FENS", "r");
    for (int i = 0; i < NPOSITIONS; i++) {

        if (fgets(line, 256, fin) == NULL)
            exit(EXIT_FAILURE);

        // Find the result { W, L, D } => { 1.0, 0.0, 0.5 }
        if      (strstr(line, "[1.0]")) entries[i].result = 1.0;
        else if (strstr(line, "[0.0]")) entries[i].result = 0.0;
        else if (strstr(line, "[0.5]")) entries[i].result = 0.5;
        else    {printf("Cannot Parse %s\n", line); exit(EXIT_FAILURE);}

        // Resolve the position to mitigate tactics
        boardFromFEN(&thread->board, line, 0);
        qsearch(thread, &thread->pv, -MATE, MATE, 0);
        for (int pvidx = 0; pvidx < thread->pv.length; pvidx++)
            applyMove(&thread->board, thread->pv.line[pvidx], &undo);

        // Defer the set to another function
        initTunerEntry(&entries[i], &thread->board, methods);

        // Occasional reporting for total completion
        if ((i + 1) % 10000 == 0 || i == NPOSITIONS - 1)
            printf("\rSetting up Entries from FENs [%7d of %7d]", i + 1, NPOSITIONS);
    }

    fclose(fin);
}

void initTunerEntry(TEntry *entry, Board *board, TArray methods) {

    // Use the same phase calculation as evaluate()
    int phase = 24 - 4 * popcount(board->pieces[QUEEN ])
                   - 2 * popcount(board->pieces[ROOK  ])
                   - 1 * popcount(board->pieces[BISHOP])
                   - 1 * popcount(board->pieces[KNIGHT]);

    // Save time by computing phase scalars now
    entry->pfactors[MG] = 1 - phase / 24.0;
    entry->pfactors[EG] = 0 + phase / 24.0;
    entry->phase = (phase * 256 + 12) / 24;

    // Save a white POV static evaluation
    TVector coeffs; T = EmptyTrace;
    entry->seval = evaluateBoard(board, NULL, 0);
    if (board->turn == BLACK) entry->seval = -entry->seval;

    // evaluate() -> [[NTERMS][COLOUR_NB]]
    initCoefficients(coeffs);
    initTunerTuples(entry, coeffs, methods);

    // We must save the pre-mixed evaluation
    entry->eval        = T.eval;
    entry->complexity  = T.complexity;
    entry->scaleFactor = T.scaleFactor;
}

void initTunerTuples(TEntry *entry, TVector coeffs, TArray methods) {

    int ttupleMB, length = 0, tidx = 0;

    // Sum up any actively used terms
    for (int i = 0; i < NTERMS; i++)
        length += (methods[i] == NORMAL &&  coeffs[i][WHITE] - coeffs[i][BLACK] != 0.0)
               || (methods[i] != NORMAL && (coeffs[i][WHITE] != 0.0 || coeffs[i][BLACK] != 0.0));

    // Allocate additional memory if needed
    if (length > TupleStackSize) {
        TupleStackSize = STACKSIZE;
        TupleStack = calloc(STACKSIZE, sizeof(TTuple));
        ttupleMB = STACKSIZE  * sizeof(TTuple) / (1 << 20);
        printf(" Allocating Tuner Tuples [%dMB]\n", ttupleMB);
    }

    // Claim part of the Tuple Stack
    entry->tuples   = TupleStack;
    entry->ntuples  = length;
    TupleStack     += length;
    TupleStackSize -= length;

    // Finally setup each of our TTuples
    for (int i = 0; i < NTERMS; i++)
        if (   (methods[i] == NORMAL &&  coeffs[i][WHITE] - coeffs[i][BLACK] != 0.0)
            || (methods[i] != NORMAL && (coeffs[i][WHITE] != 0.0 || coeffs[i][BLACK] != 0.0)))
            entry->tuples[tidx++] = (TTuple) { i, coeffs[i][WHITE], coeffs[i][BLACK] };
}


double computeOptimalK(TEntry *entries) {

    double start = -10, end = 10, step = 1;
    double curr = start, error, best = staticEvaluationErrors(entries, start);

    printf("\n\nComputing optimal K\n");

    for (int i = 0; i < KPRECISION; i++) {

        curr = start - step;
        while (curr < end) {
            curr = curr + step;
            error = staticEvaluationErrors(entries, curr);
            if (error <= best)
                best = error, start = curr;
        }

        printf("Epoch [%d] K = %f E = %f\n", i, start, best);

        end   = start + step;
        start = start - step;
        step  = step  / 10.0;
    }

    return start;
}

double staticEvaluationErrors(TEntry *entries, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(entries[i].result - sigmoid(K, entries[i].seval), 2);
    }

    return total / (double) NPOSITIONS;
}

double tunedEvaluationErrors(TEntry *entries, TVector params, TArray methods, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(entries[i].result - sigmoid(K, linearEvaluation(&entries[i], params, methods, NULL)), 2);
    }

    return total / (double) NPOSITIONS;
}

double sigmoid(double K, double E) {
    return 1.0 / (1.0 + exp(-K * E / 400.0));
}


double linearEvaluation(TEntry *entry, TVector params, TArray methods, TGradientData *data) {

    int sign;
    double midgame, endgame;
    double normal[PHASE_NB], complexity;
    double mg[METHOD_NB] = {0}, eg[METHOD_NB] = {0};

    for (int i = 0; i < entry->ntuples; i++) {
        mg[methods[i]] += (double) entry->tuples[i].wcoeff * params[entry->tuples[i].index][MG];
        mg[methods[i]] -= (double) entry->tuples[i].bcoeff * params[entry->tuples[i].index][MG];
        eg[methods[i]] += (double) entry->tuples[i].wcoeff * params[entry->tuples[i].index][EG];
        eg[methods[i]] -= (double) entry->tuples[i].bcoeff * params[entry->tuples[i].index][EG];
    }

    normal[MG] = (double) ScoreMG(entry->eval) + mg[NORMAL];
    normal[EG] = (double) ScoreEG(entry->eval) + eg[NORMAL];
    complexity = (double) ScoreEG(entry->complexity) + eg[COMPLEXITY];
    sign       = (normal[EG] > 0.0) - (normal[EG] < 0.0);

    midgame = normal[MG];
    endgame = normal[EG] + sign * fmax(-fabs(normal[EG]), complexity);

    if (data != NULL)
        *data = (TGradientData) { normal[MG], normal[EG], complexity };

    return (midgame * (256 - entry->phase)
         +  endgame * entry->phase * entry->scaleFactor / SCALE_NORMAL) / 256;
}

void computeGradient(TEntry *entries, TVector gradient, TVector params, TArray methods, double K, int batch) {

    #pragma omp parallel shared(gradient)
    {
        TVector local = {0};

        #pragma omp for schedule(static, BATCHSIZE / NPARTITIONS)
        for (int i = batch * BATCHSIZE; i < (batch + 1) * BATCHSIZE; i++)
            updateSingleGradient(&entries[i], local, params, methods, K);

        for (int i = 0; i < NTERMS; i++) {
            gradient[i][MG] += local[i][MG];
            gradient[i][EG] += local[i][EG];
        }
    }
}

void updateSingleGradient(TEntry *entry, TVector gradient, TVector params, TArray methods, double K) {

    TGradientData data;
    double E = linearEvaluation(entry, params, methods, &data);
    double S = sigmoid(K, E);
    double A = (entry->result - S) * S * (1 - S);

    double mgBase = A * entry->pfactors[MG];
    double egBase = A * entry->pfactors[EG];
    double sign = (data.egeval > 0.0) - (data.egeval < 0.0);

    for (int i = 0; i < entry->ntuples; i++) {

        int index  = entry->tuples[i].index;
        int wcoeff = entry->tuples[i].wcoeff;
        int bcoeff = entry->tuples[i].bcoeff;

        if (methods[index] == NORMAL)
            gradient[index][MG] += mgBase * (wcoeff - bcoeff);

        if (methods[index] == NORMAL && data.egeval != 0.0 && fabs(data.egeval) >= -data.complexity)
            gradient[index][EG] += egBase * (wcoeff - bcoeff) * entry->scaleFactor / SCALE_NORMAL;

        if (methods[index] == COMPLEXITY && data.complexity >= -fabs(data.egeval))
            gradient[index][EG] += egBase * wcoeff * sign * entry->scaleFactor / SCALE_NORMAL;
    }
}


void printParameters(TVector params, TVector cparams) {

    TVector tparams;

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

void print_0(char *name, TVector params, int i, char *S) {

    printf("const int %s%s = S(%4d,%4d);\n\n", name, S, (int) params[i][MG], (int) params[i][EG]);

}

void print_1(char *name, TVector params, int i, int A, char *S) {

    printf("const int %s%s = { ", name, S);

    if (A >= 3) {

        for (int a = 0; a < A; a++, i++) {
            if (a % 4 == 0) printf("\n    ");
            printf("S(%4d,%4d), ", (int) params[i][MG], (int) params[i][EG]);
        }

        printf("\n};\n\n");
    }

    else {

        for (int a = 0; a < A; a++, i++) {
            printf("S(%4d,%4d)", (int) params[i][MG], (int) params[i][EG]);
            if (a != A - 1) printf(", "); else printf(" };\n\n");
        }
    }

}

void print_2(char *name, TVector params, int i, int A, int B, char *S) {

    printf("const int %s%s = {\n", name, S);

    for (int a = 0; a < A; a++) {

        printf("   {");

        for (int b = 0; b < B; b++, i++) {
            if (b && b % 4 == 0) printf("\n    ");
            printf("S(%4d,%4d)", (int) params[i][MG], (int) params[i][EG]);
            printf("%s", b == B - 1 ? "" : ", ");
        }

        printf("},\n");
    }

    printf("};\n\n");

}

void print_3(char *name, TVector params, int i, int A, int B, int C, char *S) {

    printf("const int %s%s = {\n", name, S);

    for (int a = 0; a < A; a++) {

        for (int b = 0; b < B; b++) {

            printf("%s", b ? "   {" : "  {{");;

            for (int c = 0; c < C; c++, i++) {
                if (c &&  c % 4 == 0) printf("\n    ");
                printf("S(%4d,%4d)", (int) params[i][MG], (int) params[i][EG]);
                printf("%s", c == C - 1 ? "" : ", ");
            }

            printf("%s", b == B - 1 ? "}},\n" : "},\n");
        }

    }

    printf("};\n\n");
}

#endif