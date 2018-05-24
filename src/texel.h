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

#define NCORES      (      4) // # of Cores
#define NPOSITONS   (1400000) // # of FENS
#define NTERMS      (    497) // # of Total Terms
#define NNTERMS     (    487) // # of Normal Terms
#define NKSTERMS    (     10) // # of King Safety Terms

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
#define TunePawnIsolated                (0)
#define TunePawnStacked                 (0)
#define TunePawnBackwards               (0)
#define TunePawnConnected32             (0)
#define TuneKnightRammedPawns           (0)
#define TuneKnightOutpost               (0)
#define TuneKnightMobility              (0)
#define TuneBishopPair                  (0)
#define TuneBishopRammedPawns           (0)
#define TuneBishopOutpost               (0)
#define TuneBishopMobility              (0)
#define TuneRookFile                    (0)
#define TuneRookOnSeventh               (0)
#define TuneRookMobility                (0)
#define TuneQueenMobility               (0)
#define TuneKingDefenders               (0)
#define TuneKingShelter                 (1)
#define TunePassedPawn                  (0)
#define TuneThreatPawnAttackedByOne     (0)
#define TuneThreatMinorAttackedByPawn   (0)
#define TuneThreatMinorAttackedByMajor  (0)
#define TuneThreatQueenAttackedByOne    (0)
#define TuneTempo                       (0)
#define TuneKingSafetyBaseLine          (0)
#define TuneKingSafetyThreatWeight      (0)
#define TuneKingSafetyWeakSquares       (0)
#define TuneKingSafetyFriendlyPawns     (0)
#define TuneKingSafetyNoEnemyQueens     (0)

struct TexelEntry {
    float result;
    short eval;
    short phase;
    short kingSafety[COLOUR_NB];
    short evalCoeffs[NTERMS];
    short ksCoeffs[NTERMS][COLOUR_NB];
};

#define SET_ENABLED(term, length) do {                              \
    if (Tune##term)                                                 \
        for (int _i = 0; _i < length; _i++, i++, terms++)           \
            enabled[i] = 1;                                         \
    else                                                            \
        i += length;                                                \
} while (0)

#define INIT_PARAM_0(term, length0) do {                            \
     params[i][MG] = ScoreMG(term);                                 \
     params[i][EG] = ScoreEG(term);                                 \
     i++;                                                           \
} while (0)

#define INIT_PARAM_1(term, length1) do {                            \
    for (int _a = 0; _a < length1; _a++, i++)                       \
       {params[i][MG] = ScoreMG(term[_a]);                          \
        params[i][EG] = ScoreEG(term[_a]);}                         \
} while (0)

#define INIT_PARAM_2(term, length1, length2) do {                   \
    for (int _b = 0; _b < length1; _b++)                            \
        INIT_PARAM_1(term[_b], length2);                            \
} while (0)

#define INIT_PARAM_3(term, length1, length2, length3) do {          \
    for (int _c = 0; _c < length1; _c++)                            \
        INIT_PARAM_2(term[_c], length2, length3);                   \
} while (0)

#define INIT_NORMAL_COEFF_0(term, length0) do {                     \
    te->evalCoeffs[i] = T.term[WHITE] - T.term[BLACK];              \
    i++;                                                            \
} while (0)

#define INIT_NORMAL_COEFF_1(term, length1) do {                     \
    for (int _a = 0; _a < length1; _a++, i++)                       \
        te->evalCoeffs[i] = T.term[_a][WHITE] - T.term[_a][BLACK];  \
} while (0)

#define INIT_NORMAL_COEFF_2(term, length1, length2) do {            \
    for (int _b = 0; _b < length1; _b++)                            \
        INIT_NORMAL_COEFF_1(term[_b], length2);                     \
} while (0)

#define INIT_NORMAL_COEFF_3(term, length1, length2, length3) do {   \
    for (int _c = 0; _c < length1; _c++)                            \
        INIT_NORMAL_COEFF_2(term[_c], length2, length3);            \
} while (0)

#define INIT_KS_COEFF_0(term, length0) do {                         \
    te->ksCoeffs[i][WHITE] = T.term[WHITE];                         \
    te->ksCoeffs[i][BLACK] = T.term[BLACK];                         \
    i++;                                                            \
} while (0)

#define INIT_KS_COEFF_1(term, length1) do {                         \
    for (int _a = 0; _a < length1; _a++, i++)                       \
       {te->ksCoeffs[i][WHITE] = T.term[_a][WHITE];                 \
        te->ksCoeffs[i][BLACK] = T.term[_a][BLACK];}                \
} while (0)

void runTexelTuning(Thread *thread);

int initEnabledTerms(int enabled[NTERMS]);
void initParameters(int params[NTERMS][PHASE_NB]);
void initTexelEntries(TexelEntry *tes, int params[NTERMS][PHASE_NB], Thread *thread);
void initTexelEntry(TexelEntry *te);

double computeOptimalK(TexelEntry *tes);
double completeEvaluationError(TexelEntry *tes, double K);
double evaluationError(TexelEntry *tes, int params[NTERMS][PHASE_NB], double K);
int linearEvaluation(TexelEntry *te, int params[NTERMS][PHASE_NB]);
double sigmoid(double K, double eval);

#endif
