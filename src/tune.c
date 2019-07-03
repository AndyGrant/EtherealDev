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

#include "board.h"
#include "evaluate.h"
#include "move.h"
#include "search.h"
#include "thread.h"
#include "tune.h"
#include "types.h"
#include "uci.h"
#include "zobrist.h"

// In-house Memory Allocation
int TupleStackSize = TUPLESTACKSIZE;
LinearTuple *TupleStack;

// Tap into evaluate()
extern EvalTrace T, EmptyTrace;

// Read in all the Linear Terms
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
extern const int PawnStacked;
extern const int PawnBackwards[2];
extern const int PawnConnected32[32];
extern const int KnightOutpost[2];
extern const int KnightBehindPawn;
extern const int KnightMobility[9];
extern const int BishopPair;
extern const int BishopRammedPawns;
extern const int BishopOutpost[2];
extern const int BishopBehindPawn;
extern const int BishopMobility[14];
extern const int RookFile[2];
extern const int RookOnSeventh;
extern const int RookMobility[15];
extern const int QueenMobility[28];
extern const int KingDefenders[12];
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
extern const int ThreatQueenAttackedByOne;
extern const int ThreatOverloadedPieces;
extern const int ThreatByPawnPush;

extern const int KSAttackWeight[6];
extern const int KSAttackValue;
extern const int KSWeakSquares;
extern const int KSFriendlyPawns;
extern const int KSNoEnemyQueens;
extern const int KSSafeQueenCheck;
extern const int KSSafeRookCheck;
extern const int KSSafeBishopCheck;
extern const int KSSafeKnightCheck;
extern const int KSAdjustment;

void runTexelTuning(Thread *thread) {

    TuningEntry *tes;
    int iterations = -1;
    double K, error, best = 1e6, rate = LEARNINGRATE;
    LinearVector lparams = {0}, cparams = {0};
    SafetyVector ksparams = {0};

    setvbuf(stdout, NULL, _IONBF, 0);

    printf("\nTuner Will Be Tuning %d Linear Terms", NLINEARTERMS);
    printf("\nTuner Will Be Tuning %d Safety Terms\n", NSAFETYTERMS);

    printf("\nAllocating Memory For %d Tuning Entries [%dMB]",
        NPOSITIONS, (int)(NPOSITIONS * sizeof(TuningEntry) / (1024 * 1024)));
    tes = calloc(NPOSITIONS, sizeof(TuningEntry));

    printf("\nAllocating Memory For The Linear Tuple Stack [%dMB]\n",
        (int)(TUPLESTACKSIZE * sizeof(LinearTuple) / (1024 * 1024)));
    TupleStack = calloc(TUPLESTACKSIZE, sizeof(LinearTuple));

    printf("\nInitializing Linear Parameter Values");
    printf("\nInitializing Safety Parameter Values\n");
    initCurrentParameters(cparams, ksparams);

    printf("\nInitializing Tuning Entries");
    initTuningEntries(thread, tes, ksparams);

    printf("\n\nComputing an Optimal K Value\n");
    K = initOptimalKValue(tes);

    while (1) {

        // Shuffle up the Tuning Entries
       for (int i = 0; i < NPOSITIONS; i++) {

            int A = rand64() % NPOSITIONS;
            int B = rand64() % NPOSITIONS;

            TuningEntry temp = tes[A];
            tes[A] = tes[B];
            tes[B] = temp;
        }

        // Report every REPORTRATE iterations
        if (++iterations % REPORTRATE == 0) {

            // Check for a regression in tuning
            error = fullEvaluationError(tes, lparams, ksparams, K);
            if (error > best) rate = rate / LRDROPRATE;

            // Report the most recent parameters
            // printParameters(cparams, lparams, ksparams);
            printf("\nIteration [%d] Error = %g\n", iterations, error);
            best = error;

            for (int i = 0; i < NSAFETYTERMS; i++)
                printf(" %d ", (int)round(ksparams[i]));
            printf("\n");
        }

        // Update Linear & Safety Parameters using mini-batches
        for (int batch = 0; batch < NPOSITIONS / BATCHSIZE; batch++)
            updateParameters(tes, lparams, ksparams, K, rate, batch);
    }
}

void initCurrentParameters(LinearVector cparams, SafetyVector ksparams) {

    int i = 0; // EXECUTE_ON_LINEAR_TERMS will update i
    EXECUTE_ON_LINEAR_TERMS(INIT_LINEAR_PARAM, cparams);
    if (i != NLINEARTERMS) {
        printf("\nERROR : initCurrentParameters(): i = %d NLINEARTERMS = %d\n", i, NLINEARTERMS);
        exit(EXIT_FAILURE);
    }

    i = 0; // EXECUTE_ON_SAFETY_TERMS will update i
    EXECUTE_ON_SAFETY_TERMS(INIT_SAFETY_PARAM, ksparams);
    if (i != NSAFETYTERMS) {
        printf("\nERROR : initCurrentParameters(): i = %d NSAFETYTERMS = %d\n", i, NSAFETYTERMS);
        exit(EXIT_FAILURE);
    }
}

void initTuningEntries(Thread *thread, TuningEntry *tes, SafetyVector ksparams) {

    Limits limits;
    char line[128];
    FILE *fin = fopen(FENFILENAME, "r");

    // Make sure we don't kill the search
    thread->limits = &limits; thread->depth = 0;

    // Init a TuningEntry for each position
    for (int i = 0; i < NPOSITIONS; i++) {

        // Occasionally report progress
        if ((i + 1) % 100000 == 0 || i == NPOSITIONS - 1)
            printf("\rInitializing Tuning Entries [%7d of %7d]", i + 1, NPOSITIONS);

        // Read next position from the FEN file
        if (fgets(line, 128, fin) == NULL) {
            printf("Unable To Read Line #%d", i);
            exit(EXIT_FAILURE);
        }

        // Parse the result of the game from the line
        if      (strstr(line, "1-0")) tes[i].result = 1.0;
        else if (strstr(line, "0-1")) tes[i].result = 0.0;
        else if (strstr(line, "1/2")) tes[i].result = 0.5;
        else    {printf("Cannot Parse %s\n", line); exit(EXIT_FAILURE);}

        // Resolve the position to a quiet one
        boardFromFEN(&thread->board, line, 0);
        qsearch(thread, &thread->pv, -MATE, MATE, 0);
        for (int j = 0; j < thread->pv.length; j++)
            apply(thread, &thread->board, thread->pv.line[j], 0);

        // Fetch the overall evaluation from White's point of view.
        // This will also vectorize the entire evaluation function.
        T = EmptyTrace;
        tes[i].eval = evaluateBoard(&thread->board, NULL);
        if (thread->board.turn == BLACK) tes[i].eval *= -1;
        tes[i].turn = thread->board.turn;

        // Save off information for scaling and phase interpolation
        tes[i].scaleFactor = evaluateScaleFactor(&thread->board);
        tes[i].phase = evaluatePhase(&thread->board);
        tes[i].phaseFactors[MG] = 1 - tes[i].phase / 24.0;
        tes[i].phaseFactors[EG] = 0 + tes[i].phase / 24.0;

        // Compress and store the Linear coefficients as a sparse list
        // of Tuples. Also, save Safety coefficients for both players
        initCoefficientTuples(&tes[i]);

        // Remove Tempo from the evaluation, since it is applied
        // after the scale factor. Also, remove both King Safety
        // evals, as they must be completely recomputed each time
        tes[i].linearEval  = tes[i].eval;
        tes[i].linearEval -= tes[i].turn == WHITE ? Tempo : -Tempo;
        tes[i].linearEval -= safetyEvaluation(&tes[i], ksparams, WHITE);
        tes[i].linearEval += safetyEvaluation(&tes[i], ksparams, BLACK);
    }

    fclose(fin);
}

void initCoefficientTuples(TuningEntry *te) {

    double coeffs[NLINEARTERMS];

    int i = 0, j; // EXECUTE_ON_LINEAR_TERMS will update i
    EXECUTE_ON_LINEAR_TERMS(INIT_LINEAR_COEFF, coeffs);
    if (i != NLINEARTERMS) {
        printf("\nERROR : initCoefficientTuples(): i = %d NLINEARTERMS = %d\n", i, NLINEARTERMS);
        exit(EXIT_FAILURE);
    }

    // Count up the total number of non-zero coefficients
    for (i = 0; i < NLINEARTERMS; i++)
        te->ntuples += (coeffs[i] != 0);

    // Allocate more Linear Tuples if needed
    if (te->ntuples > TupleStackSize) {
        printf("\nAllocating Memory For The Linear Tuple Stack [%dMB]\n",
            (int)(TUPLESTACKSIZE * sizeof(LinearTuple) / (1024 * 1024)));
        TupleStack = calloc(TUPLESTACKSIZE, sizeof(LinearTuple));
        TupleStackSize = TUPLESTACKSIZE;
    }

    // Allocate the Linear Tuples
    te->linear = TupleStack;
    TupleStack += te->ntuples;
    TupleStackSize -= te->ntuples;

    // Init the Linear Tuples
    for (i = 0, j = 0; i < NLINEARTERMS; i++) {
        if (coeffs[i] != 0) {
            te->linear[j  ].index = i;
            te->linear[j++].coeff = coeffs[i];
        }
    }

    i = 0; // EXECUTE_ON_SAFETY_TERMS will update i
    EXECUTE_ON_SAFETY_TERMS(INIT_SAFETY_COEFF, te->safety);
    if (i != NSAFETYTERMS) {
        printf("\nERROR : initCoefficientTuples(): i = %d NSAFETYTERMS = %d\n", i, NSAFETYTERMS);
        exit(EXIT_FAILURE);
    }
}

double initOptimalKValue(TuningEntry *tes) {

    double start = -10.0, end = 10.0, delta = 1.0;
    double curr = start, error, best = fastFullEvaluationError(tes, start);

    for (int i = 0; i < KITERATIONS; i++) {

        curr = start - delta;
        while (curr < end) {
            curr = curr + delta;
            error = fastFullEvaluationError(tes, curr);
            if (error <= best)
                best = error, start = curr;
        }

        printf("Iteration [%d] K = %f E = %f\n", i, start, best);

        end = start + delta;
        start = start - delta;
        delta = delta / 10.0;
    }

    return start;

}

void updateParameters(TuningEntry *tes, LinearVector lparams, SafetyVector ksparams, double K, double rate, int batch) {

    int start = batch * BATCHSIZE;
    int end   = start + BATCHSIZE;

    double linearGradient[NLINEARTERMS][PHASE_NB] = {0};
    double safetyGradient[NSAFETYTERMS] = {0};

    #pragma omp parallel shared(linearGradient, safetyGradient)
    {
        double localLinearGradient[NLINEARTERMS][PHASE_NB] = {0};
        double localSafetyGradient[NSAFETYTERMS] = {0};

        #pragma omp for schedule(static, BATCHSIZE / NPARTITIONS)
        for (int i = start; i < end; i++) {

            double error = singleEvaluationError(&tes[i], lparams, ksparams, K);

            double wcount = 0, bcount = 0;
            for (int j = 0; j < NSAFETYTERMS; j++) {
                wcount += tes[i].safety[j][WHITE] * ksparams[j];
                bcount += tes[i].safety[j][BLACK] * ksparams[j];
            }

            // Update the Gradients for the used Linear terms
            for (int j = 0; j < tes[i].ntuples; j++) {

                // Read out the Linear Tuple
                int index = tes[i].linear[j].index;
                int coeff = tes[i].linear[j].coeff;

                // Adjust for game phase. EG is scaled with a factor
                int mgfactor = tes[i].phaseFactors[MG];
                int egfactor = tes[i].phaseFactors[EG] * tes[i].scaleFactor / SCALE_NORMAL;

                // Update the actual gradients
                localLinearGradient[index][MG] += error * coeff * mgfactor;
                localLinearGradient[index][EG] += error * coeff * egfactor;
            }

            // Update the Gradients for the (USED) Safety Terms
            for (int j = 0; j < NSAFETYTERMS; j++) {

                int mgcoeff = -2 * ( (wcount * tes[i].safety[j][WHITE])
                                   - (bcount * tes[i].safety[j][BLACK])) / 720;
                int egcoeff = -1 * (tes[i].safety[j][WHITE] - tes[i].safety[j][BLACK]) / 20;

                int mgfactor = tes[i].phaseFactors[MG];
                int egfactor = tes[i].phaseFactors[EG] * tes[i].scaleFactor / SCALE_NORMAL;

                localSafetyGradient[j] += error * (mgcoeff * mgfactor + egcoeff * egfactor);
            }
        }


        // Collapse Linear Gradients
        for (int i = 0; i < NLINEARTERMS; i++)
            for (int j = MG; j <= EG; j++)
                linearGradient[i][j] += localLinearGradient[i][j];

        // Collapse Safety Gradients
        for (int i = 0; i < NSAFETYTERMS; i++)
            safetyGradient[i] += localSafetyGradient[i];
    }

    // Update Linear Parameters
    for (int i = 0; i < NLINEARTERMS; i++)
        for (int j = MG; j <= EG; j++)
            lparams[i][j] += (2.0 / BATCHSIZE) * rate * linearGradient[i][j];

    // Update Safety Parameters
    for (int i = 0; i < NSAFETYTERMS; i++)
        ksparams[i] += (2.0 / BATCHSIZE) * rate * safetyGradient[i];
}

double linearEvaluation(TuningEntry *te, LinearVector lparams) {

    double mg = 0, eg = 0;

    for (int i = 0; i < te->ntuples; i++) {
        mg += te->linear[i].coeff * lparams[te->linear[i].index][MG];
        eg += te->linear[i].coeff * lparams[te->linear[i].index][EG];
    }

    return ((mg * (256 - te->phase) + eg * te->phase * te->scaleFactor / SCALE_NORMAL) / 256.0);
}

double safetyEvaluation(TuningEntry *te, SafetyVector ksparams, int colour) {

    double count = 0, mg = 0, eg = 0;
    for (int i = 0; i < NSAFETYTERMS; i++)
        count += te->safety[i][colour] * ksparams[i];

    if (count > 0) {
        mg = -count * count / 720;
        eg = -count / 20;
    }

    return ((mg * (256 - te->phase) + eg * te->phase * te->scaleFactor / SCALE_NORMAL) / 256.0);
}

double evaluate(TuningEntry *te, LinearVector lparams, SafetyVector ksparams) {

    return te->linearEval
        +  linearEvaluation(te, lparams)
        + (te->turn == WHITE ? Tempo : -Tempo)
        +  safetyEvaluation(te, ksparams, WHITE)
        -  safetyEvaluation(te, ksparams, BLACK);
}

double fastFullEvaluationError(TuningEntry *tes, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(tes[i].result - sigmoid(K, tes[i].eval), 2);
    }

    return total / (double)NPOSITIONS;
}

double fullEvaluationError(TuningEntry *tes, LinearVector lparams, SafetyVector ksparams, double K) {

    double total = 0.0;

    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NPOSITIONS / NPARTITIONS) reduction(+:total)
        for (int i = 0; i < NPOSITIONS; i++)
            total += pow(tes[i].result - sigmoid(K, evaluate(&tes[i], lparams, ksparams)), 2);
    }

    return total / (double)NPOSITIONS;
}

double singleEvaluationError(TuningEntry *te, LinearVector lparams, SafetyVector ksparams, double K) {
    double sigm  = sigmoid(K, evaluate(te, lparams, ksparams));
    double sigmp = sigm * (1 - sigm);
    return (te->result - sigm) * sigmp;
}

double sigmoid(double K, double eval) {
    return 1.0 / (1.0 + pow(10.0, -K * eval / 400.0));
}

#endif