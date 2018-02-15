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

#include "bitutils.h"
#include "board.h"
#include "evaluate.h"
#include "history.h"
#include "move.h"
#include "search.h"
#include "square.h"
#include "thread.h"
#include "texel.h"
#include "transposition.h"
#include "types.h"
#include "uci.h"

// Our own memory managment for Texel Tuning
TexelTuple* TupleStack;
int TupleStackSize;
int TupleStackOriginalSize;

// Hack so we can lower the table size for speed
extern TransTable Table;

// Need these in order to get the coefficients
// for each of the evaluation terms
extern EvalTrace T, EmptyTrace;

// To determine the starting values for the Pawn terms
extern const int PawnValue[PHASE_NB];
extern const int PawnPSQT32[32][PHASE_NB];
extern const int PawnIsolated[PHASE_NB];
extern const int PawnStacked[PHASE_NB];
extern const int PawnBackwards[2][PHASE_NB];
extern const int PawnConnected32[32][PHASE_NB];

// To determine the starting values for the Knight terms
extern const int KnightValue[PHASE_NB];
extern const int KnightPSQT32[32][PHASE_NB];
extern const int KnightAttackedByPawn[PHASE_NB];
extern const int KnightOutpost[2][PHASE_NB];
extern const int KnightMobility[9][PHASE_NB];

// To determine the starting values for the Bishop terms
extern const int BishopValue[PHASE_NB];
extern const int BishopPSQT32[32][PHASE_NB];
extern const int BishopWings[PHASE_NB];
extern const int BishopPair[PHASE_NB];
extern const int BishopAttackedByPawn[PHASE_NB];
extern const int BishopOutpost[2][PHASE_NB];
extern const int BishopMobility[14][PHASE_NB];

// To determine the starting values for the Rook terms
extern const int RookValue[PHASE_NB];
extern const int RookPSQT32[32][PHASE_NB];
extern const int RookFile[2][PHASE_NB];
extern const int RookOnSeventh[PHASE_NB];
extern const int RookMobility[15][PHASE_NB];

// To determine the starting values for the Queen terms
extern const int QueenValue[PHASE_NB];
extern const int QueenChecked[PHASE_NB];
extern const int QueenCheckedByPawn[PHASE_NB];
extern const int QueenPSQT32[32][PHASE_NB];
extern const int QueenMobility[28][PHASE_NB];

// To determine the starting values for the King terms
extern const int KingPSQT32[32][PHASE_NB];
extern const int KingDefenders[12][PHASE_NB];
extern const int KingShelter[2][FILE_NB][RANK_NB][PHASE_NB];

// To determine the starting values for the Passed Pawn terms
extern const int PassedPawn[2][2][RANK_NB][PHASE_NB];


void runTexelTuning(Thread* thread){
    
    TexelEntry* tes;
    int i, j, iteration = -1;
    float K, thisError, baseRate = 10.0;
    float rates[NT][PHASE_NB] = {{0}, {0}};
    float params[NT][PHASE_NB] = {{0}, {0}};
    float cparams[NT][PHASE_NB] = {{0}, {0}};
    
    setvbuf(stdout, NULL, _IONBF, 0);
    
    printf("\nSetting Transposition Table to 1MB...");
    initializeTranspositionTable(&Table, 1);
    
    printf("\n\nAllocating Memory for Texel Tuner Structs [%dMB]...",
           (int)(NP * sizeof(TexelEntry) / (1024 * 1024)));
    tes = calloc(NP, sizeof(TexelEntry));
    
    printf("\n\nAllocating Memory for Texel Tuple Stack [%dMB]...",
            (int)(StackSize * sizeof(TexelTuple) / (1024 * 1024)));
    TupleStack = calloc(StackSize, sizeof(TexelTuple));
    TupleStackOriginalSize = TupleStackSize = StackSize;
    
    printf("\n\nReading and Initializing Texel Entries from FENS...");
    initializeTexelEntries(tes, thread);
    
    printf("\n\nFetching Current Evaluation Terms as a Starting Point...");
    initializeCurrentParameters(cparams);
    
    printf("\n\nScaling Params For Phases and Occurance Rates...");
    calculateLearningRates(tes, rates);
    
    printf("\n\nComputing Optimal K Value...");
    K = computeOptimalK(tes);
    
    while (1){
        
        iteration++;
        
        if (iteration % 100 == 0){
            printParameters(params, cparams);
            printf("\nIteration [%d] Error = %g \n", iteration, completeLinearError(tes, params, K));
        }
                
        float gradients[NT][PHASE_NB] = {{0}, {0}};
        #pragma omp parallel shared(gradients)
        {
            float localgradients[NT][PHASE_NB] = {{0}, {0}};
            #pragma omp for schedule(static, NP / 48)
            for (i = 0; i < NP; i++){
                thisError = singleLinearError(tes[i], params, K);
                
                // Update the gradients for each of the terms used in this position
                for (j = 0; j < tes[i].ntuples; j++){

                    localgradients[tes[i].tuples[j].index][MG] += thisError 
                                                                * tes[i].tuples[j].coeff
                                                                * tes[i].factors[MG];

                    localgradients[tes[i].tuples[j].index][EG] += thisError 
                                                                * tes[i].tuples[j].coeff 
                                                                * tes[i].factors[EG];
                }
            }
            
            for (i = 0; i < NT; i++){
                gradients[i][MG] += localgradients[i][MG];
                gradients[i][EG] += localgradients[i][EG];
            }
        }
        
        for (i = 0; i < NT; i++){
            params[i][MG] += (2.0 / NP) * baseRate * rates[i][MG] * gradients[i][MG];
            params[i][EG] += (2.0 / NP) * baseRate * rates[i][EG] * gradients[i][EG];
        }
    }
}

void initializeTexelEntries(TexelEntry* tes, Thread* thread){
    
    int i, j, k;
    Undo undo;
    EvalInfo ei;
    Limits limits;
    char line[128];
    int coeffs[NT];
    
    // Initialize limits for the search
    limits.limitedByNone  = 0;
    limits.limitedByTime  = 0;
    limits.limitedByDepth = 1;
    limits.limitedBySelf  = 0;
    limits.timeLimit      = 0;
    limits.depthLimit     = 1;
    
    // Initialize the thread for the search
    thread->limits = &limits;
    thread->depth  = 1;
    thread->abort  = 0;
    
    FILE * fin = fopen("FENS", "r");
    
    for (i = 0; i < NP; i++){
        
        if ((i + 1) % 100000 == 0 || i == NP - 1)
            printf("\rReading and Initializing Texel Entries from FENS...  [%7d of %7d]", i + 1, NP);
        
        fgets(line, 128, fin);
        
           // Determine the result of the game
        if      (strstr(line, "1-0")) tes[i].result = 1.0;
        else if (strstr(line, "1/2")) tes[i].result = 0.5;
        else if (strstr(line, "0-1")) tes[i].result = 0.0;
        else    {printf("Unable to Parse Result\n"); exit(0);}
        
        // Search, then and apply all moves in the principle variation
        initializeBoard(&thread->board, line);
        // search(thread, &thread->pv, -MATE, MATE, 1, 0);
        // for (j = 0; j < thread->pv.length; j++)
        //     applyMove(&thread->board, thread->pv.line[j], &undo);
            
        // Get the eval trace for the final position in the pv
        T = EmptyTrace;
        tes[i].eval = evaluateBoard(&thread->board, &ei, NULL);
        if (thread->board.turn == BLACK) tes[i].eval *= -1;
        
        // Determine the game phase based on remaining material
        tes[i].phase = 24 - 4 * popcount(thread->board.pieces[QUEEN ])
                          - 2 * popcount(thread->board.pieces[ROOK  ])
                          - 1 * popcount(thread->board.pieces[KNIGHT])
                          - 1 * popcount(thread->board.pieces[BISHOP]);
                          
        // When updating gradients, we use the coefficients for each
        // term, as well as the phase of the position it came from
        tes[i].factors[MG] = 1 - tes[i].phase / 24.0;
        tes[i].factors[EG] =     tes[i].phase / 24.0;
        
        // Finish determining the phase to match the final phase factor
        tes[i].phase = (tes[i].phase * 256 + 12) / 24.0;
        
        // Dump evaluation terms into coeffs
        initializeCoefficients(coeffs);

        // Determine how many terms actually apply to this position. We do this
        // since the vast majority of terms are not used in a given position.
        for (k = 0, j = 0; j < NT; j++)
            k += coeffs[j] != 0; 

        // We have remaining Texel Tuples on our stack. Set the tuple pointer,
        // and then update the location and size of the Tuple Stack
        if (TupleStackSize >= k){ 
            tes[i].tuples = TupleStack;
            tes[i].ntuples = k;
            TupleStack += k;
            TupleStackSize -= k;
        }
        
        // We did not have enough remaining Texel Tuples on our stack. Allocate
        // more and then setup the texel tuples for this texel entry
        else {
            TupleStack = calloc(TupleStackOriginalSize, sizeof(TexelTuple));
            tes[i].tuples = TupleStack;
            tes[i].ntuples = k;
            TupleStack += k;
            TupleStackSize = TupleStackOriginalSize - k;
        }
        
        // Finally, initialize the tuples with the coefficients and index
        for (k = 0, j = 0; j < NT; j++){
            if (coeffs[j] != 0){
                
                // Initialize the Kth tuple
                tes[i].tuples[k].index = j,
                tes[i].tuples[k].coeff = coeffs[j],
                
                 // Clear coeffs for next use
                coeffs[j] = 0;
            }
        }
    }
    
    fclose(fin);
}

void initializeCoefficients(int* coeffs){
    
    int i = 0, a, b, c;
    
    // Initialize coefficients for the pawns
    
    coeffs[i++] = T.pawnCounts[WHITE] - T.pawnCounts[BLACK];
    
    for (a = 0; a < 64; a++){
        coeffs[i + relativeSquare32(a, WHITE)] += T.pawnPSQT[WHITE][a];
        coeffs[i + relativeSquare32(a, BLACK)] -= T.pawnPSQT[BLACK][a];
    } i += 32;
    
    coeffs[i++] = T.pawnIsolated[WHITE] - T.pawnIsolated[BLACK];
    
    coeffs[i++] = T.pawnStacked[WHITE] - T.pawnStacked[BLACK];
    
    for (a = 0; a < 2; a++)
        coeffs[i++] = T.pawnBackwards[WHITE][a] - T.pawnBackwards[BLACK][a];
    
    for (a = 0; a < 64; a++){
        coeffs[i + relativeSquare32(a, WHITE)] += T.pawnConnected[WHITE][a];
        coeffs[i + relativeSquare32(a, BLACK)] -= T.pawnConnected[BLACK][a];
    } i += 32;
    
    
    // Initialze coefficients for the knights
    
    coeffs[i++] = T.knightCounts[WHITE] - T.knightCounts[BLACK];
    
    for (a = 0; a < 64; a++){
        coeffs[i + relativeSquare32(a, WHITE)] += T.knightPSQT[WHITE][a];
        coeffs[i + relativeSquare32(a, BLACK)] -= T.knightPSQT[BLACK][a];
    } i += 32;
    
    coeffs[i++] = T.knightAttackedByPawn[WHITE] - T.knightAttackedByPawn[BLACK];
    
    for (a = 0; a < 2; a++)
        coeffs[i++] = T.knightOutpost[WHITE][a] - T.knightOutpost[BLACK][a];
        
    for (a = 0; a < 9; a++)
        coeffs[i++] = T.knightMobility[WHITE][a] - T.knightMobility[BLACK][a];
    
    
    // Initialize coefficients for the bishops
    
    coeffs[i++] = T.bishopCounts[WHITE] - T.bishopCounts[BLACK];
    
    for (a = 0; a < 64; a++){
        coeffs[i + relativeSquare32(a, WHITE)] += T.bishopPSQT[WHITE][a];
        coeffs[i + relativeSquare32(a, BLACK)] -= T.bishopPSQT[BLACK][a];
    } i += 32;
    
    coeffs[i++] = T.bishopWings[WHITE] - T.bishopWings[BLACK];
    
    coeffs[i++] = T.bishopPair[WHITE] - T.bishopPair[BLACK];
    
    coeffs[i++] = T.bishopAttackedByPawn[WHITE] - T.bishopAttackedByPawn[BLACK];
    
    for (a = 0; a < 2; a++)
        coeffs[i++] = T.bishopOutpost[WHITE][a] - T.bishopOutpost[BLACK][a];
        
    for (a = 0; a < 14; a++)
        coeffs[i++] = T.bishopMobility[WHITE][a] - T.bishopMobility[BLACK][a];
    
    
    // Initialize coefficients for the rooks
    
    coeffs[i++] = T.rookCounts[WHITE] - T.rookCounts[BLACK];
    
    for (a = 0; a < 64; a++){
        coeffs[i + relativeSquare32(a, WHITE)] += T.rookPSQT[WHITE][a];
        coeffs[i + relativeSquare32(a, BLACK)] -= T.rookPSQT[BLACK][a];
    } i += 32;
    
    for (a = 0; a < 2; a++)
        coeffs[i++] = T.rookFile[WHITE][a] - T.rookFile[BLACK][a];
        
    coeffs[i++] = T.rookOnSeventh[WHITE] - T.rookOnSeventh[BLACK];
        
    for (a = 0; a < 15; a++)
        coeffs[i++] = T.rookMobility[WHITE][a] - T.rookMobility[BLACK][a];
    
    
    // Initialize coefficients for the queens
    
    coeffs[i++] = T.queenCounts[WHITE] - T.queenCounts[BLACK];
    
    coeffs[i++] = T.queenChecked[WHITE] - T.queenChecked[BLACK];
    
    coeffs[i++] = T.queenCheckedByPawn[WHITE] - T.queenCheckedByPawn[BLACK];
    
    for (a = 0; a < 64; a++){
        coeffs[i + relativeSquare32(a, WHITE)] += T.queenPSQT[WHITE][a];
        coeffs[i + relativeSquare32(a, BLACK)] -= T.queenPSQT[BLACK][a];
    } i += 32;
    
    for (a = 0; a < 28; a++)
        coeffs[i++] = T.queenMobility[WHITE][a] - T.queenMobility[BLACK][a];
    
    
    // Intitialize coefficients for the kings
    
    for (a = 0; a < 64; a++){
        coeffs[i + relativeSquare32(a, WHITE)] += T.kingPSQT[WHITE][a];
        coeffs[i + relativeSquare32(a, BLACK)] -= T.kingPSQT[BLACK][a];
    } i += 32;
    
    for (a = 0; a < 12; a++)
        coeffs[i++] = T.kingDefenders[WHITE][a] - T.kingDefenders[BLACK][a];
    
    for (a = 0; a < 2; a++)
        for (b = 0; b < 2; b++)
            for (c = 0; c < RANK_NB; c++)
                coeffs[i++] = T.kingShelter[WHITE][a][b][c] - T.kingShelter[BLACK][a][b][c];
    
    // Initialize coefficients for the passed pawns
    
    for (a = 0; a < 2; a++)
        for (b = 0; b < 2; b++)
            for (c = 0; c < RANK_NB; c++)
                coeffs[i++] = T.passedPawn[WHITE][a][b][c] - T.passedPawn[BLACK][a][b][c];
}

void initializeCurrentParameters(float cparams[NT][PHASE_NB]){
    
    int i = 0, a, b, c;
    
    // Initialize parameters for the pawns
    
    cparams[i  ][MG] = PawnValue[MG];
    cparams[i++][EG] = PawnValue[EG];
    
    for (a = 0; a < 32; a++, i++){
        cparams[i][MG] = PawnPSQT32[a][MG];
        cparams[i][EG] = PawnPSQT32[a][EG];
    }
    
    cparams[i  ][MG] = PawnIsolated[MG];
    cparams[i++][EG] = PawnIsolated[EG];
    
    cparams[i  ][MG] = PawnStacked[MG];
    cparams[i++][EG] = PawnStacked[EG];
    
    for (a = 0; a < 2; a++, i++){
        cparams[i][MG] = PawnBackwards[a][MG];
        cparams[i][EG] = PawnBackwards[a][EG];
    }
    
    for (a = 0; a < 32; a++, i++){
        cparams[i][MG] = PawnConnected32[a][MG];
        cparams[i][EG] = PawnConnected32[a][EG];
    }
    
    
    // Initialize parameters for the knights
    
    cparams[i  ][MG] = KnightValue[MG];
    cparams[i++][EG] = KnightValue[EG];
    
    for (a = 0; a < 32; a++, i++){
        cparams[i][MG] = KnightPSQT32[a][MG];
        cparams[i][EG] = KnightPSQT32[a][EG];
    }
    
    cparams[i  ][MG] = KnightAttackedByPawn[MG];
    cparams[i++][EG] = KnightAttackedByPawn[EG];
    
    for (a = 0; a < 2; a++, i++){
        cparams[i][MG] = KnightOutpost[a][MG];
        cparams[i][EG] = KnightOutpost[a][EG];
    }
    
    for (a = 0; a < 9; a++, i++){
        cparams[i][MG] = KnightMobility[a][MG];
        cparams[i][EG] = KnightMobility[a][EG];
    }
    
    
    // Initialize parameters for the bishops
    
    cparams[i  ][MG] = BishopValue[MG];
    cparams[i++][EG] = BishopValue[EG];
    
    for (a = 0; a < 32; a++, i++){
        cparams[i][MG] = BishopPSQT32[a][MG];
        cparams[i][EG] = BishopPSQT32[a][EG];
    }
    
    cparams[i  ][MG] = BishopWings[MG];
    cparams[i++][EG] = BishopWings[EG];
    
    cparams[i  ][MG] = BishopPair[MG];
    cparams[i++][EG] = BishopPair[EG];
    
    cparams[i  ][MG] = BishopAttackedByPawn[MG];
    cparams[i++][EG] = BishopAttackedByPawn[EG];
    
    for (a = 0; a < 2; a++, i++){
        cparams[i][MG] = BishopOutpost[a][MG];
        cparams[i][EG] = BishopOutpost[a][EG];
    }
    
    for (a = 0; a < 14; a++, i++){
        cparams[i][MG] = BishopMobility[a][MG];
        cparams[i][EG] = BishopMobility[a][EG];
    }
    
    
    // Initialize parameters for the rooks
    
    cparams[i  ][MG] = RookValue[MG];
    cparams[i++][EG] = RookValue[EG];
    
    for (a = 0; a < 32; a++, i++){
        cparams[i][MG] = RookPSQT32[a][MG];
        cparams[i][EG] = RookPSQT32[a][EG];
    }
    
    for (a = 0; a < 2; a++, i++){
        cparams[i][MG] = RookFile[a][MG];
        cparams[i][EG] = RookFile[a][EG];
    }
    
    cparams[i  ][MG] = RookOnSeventh[MG];
    cparams[i++][EG] = RookOnSeventh[EG];
    
    for (a = 0; a < 15; a++, i++){
        cparams[i][MG] = RookMobility[a][MG];
        cparams[i][EG] = RookMobility[a][EG];
    }
    
    
    // Initialize parameters for the queens
    
    cparams[i  ][MG] = QueenValue[MG];
    cparams[i++][EG] = QueenValue[EG];
    
    cparams[i  ][MG] = QueenChecked[MG];
    cparams[i++][EG] = QueenChecked[EG];
    
    cparams[i  ][MG] = QueenCheckedByPawn[MG];
    cparams[i++][EG] = QueenCheckedByPawn[EG];
    
    for (a = 0; a < 32; a++, i++){
        cparams[i][MG] = QueenPSQT32[a][MG];
        cparams[i][EG] = QueenPSQT32[a][EG];
    }
    
    for (a = 0; a < 28; a++, i++){
        cparams[i][MG] = QueenMobility[a][MG];
        cparams[i][EG] = QueenMobility[a][EG];
    }
    
    
    // Initialize parameters for the kings
    
    for (a = 0; a < 32; a++, i++){
        cparams[i][MG] = KingPSQT32[a][MG];
        cparams[i][EG] = KingPSQT32[a][EG];
    }
    
    for (a = 0; a < 12; a++, i++){
        cparams[i][MG] = KingDefenders[a][MG];
        cparams[i][EG] = KingDefenders[a][EG];
    }
    
    for (a = 0; a < 2; a++){
        for (b = 0; b < FILE_NB; b++){
            for (c = 0; c < RANK_NB; c++, i++){
                cparams[i][MG] = KingShelter[a][b][c][MG];
                cparams[i][EG] = KingShelter[a][b][c][EG];
            }
        }
    }
    
    // Initialize parameters for the passed pawns
    
    for (a = 0; a < 2; a++){
        for (b = 0; b < 2; b++){
            for (c = 0; c < RANK_NB; c++, i++){
                cparams[i][MG] = PassedPawn[a][b][c][MG];
                cparams[i][EG] = PassedPawn[a][b][c][EG];
            }
        }
    }
}

void calculateLearningRates(TexelEntry* tes, float rates[NT][PHASE_NB]){
    
    int i, j;
    float avgByPhase[PHASE_NB] = {0};
    float occurances[NT][PHASE_NB] = {{0}, {0}};
    
    for (i = 0; i < NP; i++){
        for (j = 0; j < tes[i].ntuples; j++){
            occurances[tes[i].tuples[j].index][MG] += abs(tes[i].tuples[j].coeff) * tes[i].factors[MG];
            occurances[tes[i].tuples[j].index][EG] += abs(tes[i].tuples[j].coeff) * tes[i].factors[EG];
            avgByPhase[MG] += abs(tes[i].tuples[j].coeff) * tes[i].factors[MG];
            avgByPhase[EG] += abs(tes[i].tuples[j].coeff) * tes[i].factors[EG];
        }
    }
    
    avgByPhase[MG] /= NT;
    avgByPhase[EG] /= NT;
        
    for (i = 0; i < NT; i++){
        if (occurances[i][MG] >= 1.0)
            rates[i][MG] = avgByPhase[MG] / occurances[i][MG];
        if (occurances[i][EG] >= 1.0)
            rates[i][EG] = avgByPhase[EG] / occurances[i][EG];
    }
}

void printParameters(float params[NT][PHASE_NB], float cparams[NT][PHASE_NB]){
    
    int i = 0, x, y;
    
    float tparams[NT][PHASE_NB];
    
    // We must first combine the original params and the update's
    // to the params before having the final parameter outputs
    for (x = 0; x < NT; x++){
        tparams[x][MG] = params[x][MG] + cparams[x][MG];
        tparams[x][EG] = params[x][EG] + cparams[x][EG];
    }    
    
    // Print Pawn Parameters
    
    printf("\nconst int PawnValue[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int PawnPSQT32[32][PHASE_NB] = {");
    for (x = 0; x < 8; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    printf("\nconst int PawnIsolated[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int PawnStacked[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int PawnBackwards[2][PHASE_NB] = { {%4d,%4d}, {%4d,%4d} };\n",
            (int)tparams[i  ][MG], (int)tparams[i  ][EG],
            (int)tparams[i+1][MG], (int)tparams[i+1][EG]); i += 2;
    
    printf("\nconst int PawnConnected32[32][PHASE_NB] = {");
    for (x = 0; x < 8; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    
    // Print Knight Parameters
    
    printf("\nconst int KnightValue[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int KnightPSQT32[32][PHASE_NB] = {");
    for (x = 0; x < 8; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    printf("\nconst int KnightAttackedByPawn[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int KnightOutpost[2][PHASE_NB] = { {%4d,%4d}, {%4d,%4d} };\n",
            (int)tparams[i  ][MG], (int)tparams[i  ][EG],
            (int)tparams[i+1][MG], (int)tparams[i+1][EG]); i += 2;
            
    printf("\nconst int KnightMobility[9][PHASE_NB] = {");
    for (x = 0; x < 3; x++){
        printf("\n   ");
        for (y = 0; y < 3; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    
    // Print Bishop Parameters
    
    printf("\nconst int BishopValue[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int BishopPSQT32[32][PHASE_NB] = {");
    for (x = 0; x < 8; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    printf("\nconst int BishopWings[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int BishopPair[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int BishopAttackedByPawn[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int BishopOutpost[2][PHASE_NB] = { {%4d,%4d}, {%4d,%4d} };\n",
            (int)tparams[i  ][MG], (int)tparams[i  ][EG],
            (int)tparams[i+1][MG], (int)tparams[i+1][EG]); i += 2;
            
    printf("\nconst int BishopMobility[14][PHASE_NB] = {");
    for (x = 0; x < 4; x++){
        printf("\n   ");
        for (y = 0; y < 4 && 4 * x + y < 14; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    
    // Print Rook Parameters
    
    printf("\nconst int RookValue[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int RookPSQT32[32][PHASE_NB] = {");
    for (x = 0; x < 8; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    printf("\nconst int RookFile[2][PHASE_NB] = { {%4d,%4d}, {%4d,%4d} };\n",
            (int)tparams[i  ][MG], (int)tparams[i  ][EG],
            (int)tparams[i+1][MG], (int)tparams[i+1][EG]); i += 2;
            
    printf("\nconst int RookOnSeventh[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
            
    printf("\nconst int RookMobility[15][PHASE_NB] = {");
    for (x = 0; x < 4; x++){
        printf("\n   ");
        for (y = 0; y < 4 && x * 4 + y < 15; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    
    // Print Queen Parameters
    
    printf("\nconst int QueenValue[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int QueenChecked[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int QueenCheckedByPawn[PHASE_NB] = {%4d,%4d};\n", (int)tparams[i][MG], (int)tparams[i][EG]); i++;
    
    printf("\nconst int QueenPSQT32[32][PHASE_NB] = {");
    for (x = 0; x < 8; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
            
    printf("\nconst int QueenMobility[28][PHASE_NB] = {");
    for (x = 0; x < 7; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    
    // Print King Parameters
    
    printf("\nconst int KingPSQT32[32][PHASE_NB] = {");
    for (x = 0; x < 8; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    printf("\nconst int KingDefenders[12][PHASE_NB] = {");
    for (x = 0; x < 3; x++){
        printf("\n   ");
        for (y = 0; y < 4; y++, i++)
            printf(" {%4d,%4d},", (int)tparams[i][MG], (int)tparams[i][EG]);
    } printf("\n};\n");
    
    printf("\nconst int KingShelter[2][FILE_NB][RANK_NB][PHASE_NB] = {");
    for (x = 0; x < 16; x++){
        printf("\n  %s", x % 8 ? " {" : "{{");
        for (y = 0; y < RANK_NB; y++, i++){
            printf("{%4d,%4d}", (int)tparams[i][MG], (int)tparams[i][EG]);
            printf("%s", y < RANK_NB - 1 ? ", " : x % 8 == 7 ? "}}," : "},");
        }
    } printf("\n};\n");
    
    // Print Passed Pawn Parameters
    
    printf("\nconst int PassedPawn[2][2][RANK_NB][PHASE_NB] = {");
    for (x = 0; x < 4; x++){
        printf("\n  %s", x % 2 ? " {" : "{{");
        for (y = 0; y < RANK_NB; y++, i++){
            printf("{%4d,%4d}", (int)tparams[i][MG], (int)tparams[i][EG]);
            printf("%s", y < RANK_NB - 1 ? ", " : x % 2 ? "}}," : "},");
        }
    } printf("\n};\n");
}

float computeOptimalK(TexelEntry* tes){
    
    int i;
    float start = -10.0, end = 10.0, delta = 1.0;
    float curr = start, thisError, bestError = completeEvaluationError(tes, start);
    
    for (i = 0; i < 6; i++){
        printf("Computing K Iteration [%d] ", i);
        
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

float completeEvaluationError(TexelEntry* tes, float K){
    
    int i;
    float total = 0.0;
    
    // Determine the error margin using the evaluation from evaluateBoard
    
    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NP / 48) reduction(+:total)
        for (i = 0; i < NP; i++)
            total += pow(tes[i].result - sigmoid(K, tes[i].eval), 2);
    }
        
    return total / (float)NP;
}

float completeLinearError(TexelEntry* tes, float params[NT][PHASE_NB], float K){
    
    int i;
    float total = 0.0;
    
    // Determine the error margin using evaluation from summing up PARAMS
    
    #pragma omp parallel shared(total)
    {
        #pragma omp for schedule(static, NP / 48) reduction(+:total)
        for (i = 0; i < NP; i++)
            total += pow(tes[i].result - sigmoid(K, linearEvaluation(tes[i], params)), 2);
    }
        
    return total / (float)NP;
}

float singleLinearError(TexelEntry te, float params[NT][PHASE_NB], float K){
    return te.result - sigmoid(K, linearEvaluation(te, params));
}

float linearEvaluation(TexelEntry te, float params[NT][PHASE_NB]){
    
    int i;
    float mg = 0, eg = 0;
    
    for (i = 0; i < te.ntuples; i++){
        mg += te.tuples[i].coeff * params[te.tuples[i].index][MG];
        eg += te.tuples[i].coeff * params[te.tuples[i].index][EG];
    }
    
    return te.eval + ((mg * (256 - te.phase) + eg * te.phase) / 256.0);
}

float sigmoid(float K, float S){
    return 1.0 / (1.0 + pow(10.0, -K * S / 400.0));
}

#endif
