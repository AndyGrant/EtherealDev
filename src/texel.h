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

#if defined(TUNE) && !defined(_TEXEL_H)
#define _TEXEL_H

#include "types.h"

#define NP (7400000)

#define NT (484)

#define StackSize ((int)((float) NP * NT / 20))

typedef struct TexelTuple {    
    int index;
    float coeff;
} TexelTuple;

typedef struct TexelEntry {
    int ntuples;
    float result;
    float eval, phase;
    float factors[PHASE_NB];
    TexelTuple* tuples;
} TexelEntry;

void runTexelTuning(Thread* thread);

void initializeTexelEntries(TexelEntry* tes, Thread* thread);

void initializeCoefficients(int* coeffs);

void initializeCurrentParameters(float cparams[NT][PHASE_NB]);

void calculateLearningRates(TexelEntry* tes, float rates[NT][PHASE_NB]);

void printParameters(float params[NT][PHASE_NB], float cparams[NT][PHASE_NB]);

float computeOptimalK(TexelEntry* tes);

float completeEvaluationError(TexelEntry* tes, float K);

float completeLinearError(TexelEntry* tes, float params[NT][PHASE_NB], float K);

float singleLinearError(TexelEntry te, float params[NT][PHASE_NB], float K);

float linearEvaluation(TexelEntry te, float params[NT][PHASE_NB]);

float sigmoid(float K, float S);

#endif
