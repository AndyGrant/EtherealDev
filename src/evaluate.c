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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "attacks.h"
#include "bitboards.h"
#include "board.h"
#include "evalcache.h"
#include "evaluate.h"
#include "move.h"
#include "masks.h"
#include "network.h"
#include "nneval.h"
#include "thread.h"
#include "transposition.h"
#include "types.h"

EvalTrace T, EmptyTrace;
int PSQT[32][SQUARE_NB];

#define S(mg, eg) (MakeScore((mg), (eg)))

/* Material Value Evaluation Terms */

const int PawnValue   = S(  87, 146);
const int KnightValue = S( 438, 482);
const int BishopValue = S( 461, 522);
const int RookValue   = S( 648, 817);
const int QueenValue  = S(1324,1699);
const int KingValue   = S(   0,   0);

/* Piece Square Evaluation Terms */

const int PawnPSQT[SQUARE_NB] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S( -13,  10), S(  -5,   1), S(   2,   5), S(   7,   2),
    S(   4,  11), S(  -9,   3), S(  -9,   2), S( -17,  11),
    S( -21,   7), S( -17,   6), S(   1,  -6), S(  14, -16),
    S(   9, -11), S(  -3,  -6), S( -14,   4), S( -25,  14),
    S( -14,  18), S( -23,  17), S(  10, -12), S(  11, -26),
    S(   5, -24), S(   6, -12), S( -20,  15), S( -17,  21),
    S( -14,  20), S( -19,  11), S( -16,  -9), S(   5, -33),
    S(  -1, -26), S( -18, -11), S( -23,  10), S( -18,  23),
    S( -22,  54), S( -12,  48), S(   1,  34), S(  21,  -9),
    S(  39,  -6), S( -11,  32), S(  -7,  46), S( -25,  56),
    S( -45, -61), S( -76,  -1), S( -21, -17), S(  72, -21),
    S(  45, -15), S(  56, -17), S( -45,   7), S( -72, -47),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT[SQUARE_NB] = {
    S( -33, -35), S(  -6, -23), S( -20, -23), S( -15,   1),
    S( -10,   0), S( -22, -18), S(  -7, -18), S( -42, -27),
    S(   1,  -2), S( -11,   3), S(  -6, -19), S(   1,  -2),
    S(   1,   0), S(  -8, -16), S(  -6,  -1), S(  -5,   3),
    S(   8, -21), S(   9,  -6), S(   8,   1), S(  12,  19),
    S(  12,  19), S(   5,   1), S(  10,  -4), S(   4, -19),
    S(  16,  22), S(  17,  32), S(  23,  42), S(  28,  52),
    S(  24,  55), S(  24,  42), S(  16,  29), S(  12,  27),
    S(  12,  32), S(  23,  31), S(  38,  53), S(  29,  72),
    S(  25,  68), S(  38,  52), S(  22,  34), S(  13,  31),
    S( -28,  27), S(  -5,  39), S(  26,  58), S(  20,  62),
    S(  24,  56), S(  26,  57), S(  -6,  35), S( -24,  27),
    S(  13,  -2), S( -13,  19), S(  28,  -4), S(  35,  23),
    S(  38,  23), S(  39,  -9), S( -16,  17), S(   0,  -3),
    S(-180,  -6), S(-100,  13), S(-129,  43), S( -44,  18),
    S( -26,  19), S(-116,  49), S(-130,  26), S(-176, -18),
};

const int BishopPSQT[SQUARE_NB] = {
    S(   7, -21), S(   3,   1), S(   0,   5), S(   3,   5),
    S(   4,   9), S(  -5,  -1), S(   1,   1), S(   7, -25),
    S(  29, -17), S(   4, -32), S(  17,  -2), S(  10,   8),
    S(  10,   8), S(  14,  -3), S(  12, -31), S(  27, -30),
    S(  11,   3), S(  25,   8), S(  -3,  -3), S(  20,  19),
    S(  18,  20), S(  -3,  -6), S(  21,   3), S(  16,   8),
    S(   1,  13), S(  12,  17), S(  18,  33), S(  21,  34),
    S(  25,  36), S(  13,  31), S(  17,  18), S(   0,  15),
    S( -19,  36), S(  15,  32), S(   1,  40), S(  22,  47),
    S(  12,  48), S(   5,  40), S(  14,  34), S( -16,  39),
    S( -12,  33), S( -10,  47), S(  -5,  26), S(   3,  42),
    S(   8,  39), S( -21,  37), S(  -7,  49), S( -10,  37),
    S( -61,  38), S( -50,  23), S( -12,  30), S( -35,  35),
    S( -34,  35), S( -15,  32), S( -70,  21), S( -59,  37),
    S( -68,  18), S( -69,  36), S(-134,  49), S(-115,  56),
    S(-121,  54), S(-101,  43), S( -38,  23), S( -80,  17),
};

const int RookPSQT[SQUARE_NB] = {
    S( -26,   1), S( -21,   5), S( -13,   6), S(  -5,  -3),
    S(  -5,  -2), S(  -9,   5), S( -13,  -1), S( -21, -14),
    S( -72,   5), S( -24, -12), S( -17,  -7), S( -10, -11),
    S(  -8, -14), S( -14, -17), S( -13, -20), S( -79,   2),
    S( -39,   1), S( -16,  16), S( -24,   8), S( -12,   1),
    S( -11,   2), S( -24,   7), S(  -3,  10), S( -38,  -2),
    S( -31,  21), S( -20,  36), S( -21,  38), S(  -4,  27),
    S(  -6,  27), S( -17,  33), S( -11,  32), S( -28,  22),
    S( -20,  46), S(   6,  35), S(  18,  39), S(  38,  31),
    S(  35,  32), S(  14,  32), S(  21,  25), S( -13,  42),
    S( -33,  61), S(  19,  41), S(  -1,  57), S(  35,  37),
    S(  32,  34), S(   6,  51), S(  39,  27), S( -25,  58),
    S( -20,  43), S( -28,  52), S(  -4,  42), S(  11,  45),
    S(  10,  43), S(  -2,  36), S( -25,  50), S( -13,  39),
    S(  28,  64), S(  19,  72), S(  -7,  82), S(   3,  75),
    S(   4,  78), S(  -4,  77), S(  34,  64), S(  37,  61),
};

const int QueenPSQT[SQUARE_NB] = {
    S(  22, -34), S(   5, -26), S(  11, -35), S(  20, -14),
    S(  20, -15), S(  16, -46), S(  10, -27), S(  25, -44),
    S(   8, -13), S(  17, -22), S(  24, -42), S(  15,   5),
    S(  19,   2), S(  25, -49), S(  21, -28), S(   4, -19),
    S(   7,   2), S(  24,   8), S(   6,  39), S(   1,  37),
    S(   3,  38), S(   5,  40), S(  26,  10), S(  14, -16),
    S(  10,  19), S(  13,  50), S(  -6,  64), S( -20, 115),
    S( -18, 113), S(  -5,  60), S(  19,  51), S(   8,  35),
    S( -10,  45), S(  -8,  82), S( -20,  71), S( -34, 128),
    S( -34, 135), S( -25,  87), S(  -9, 100), S( -11,  70),
    S( -29,  59), S( -23,  53), S( -34,  73), S( -20,  74),
    S( -19,  76), S( -21,  69), S( -21,  73), S( -29,  80),
    S( -16,  64), S( -76, 117), S( -21,  71), S( -57, 123),
    S( -58, 128), S( -16,  64), S( -73, 126), S( -12,  78),
    S(   7,  44), S(  17,  52), S(  -3,  83), S(  -3,  85),
    S(  -7,  93), S(   8,  72), S(  14,  87), S(  20,  59),
};

const int KingPSQT[SQUARE_NB] = {
    S(  89, -75), S(  65, -44), S(  -1,   0), S( -16, -24),
    S( -15, -25), S( -15,   2), S(  56, -49), S(  79, -84),
    S(  34,  12), S( -32,   4), S( -45,  23), S( -96,  35),
    S( -69,  31), S( -71,  33), S( -29,   2), S(  28,  -1),
    S( -57, -13), S( -19, -17), S(  35,   9), S(   1,  39),
    S(  23,  36), S(  31,  12), S( -12, -16), S( -38, -11),
    S( -59, -46), S( 124, -47), S(  91,  11), S( -17,  48),
    S(  22,  44), S(  90,  10), S( 132, -37), S( -51, -44),
    S( -18, -12), S( 134, -54), S(  71,   6), S( -34,  12),
    S( -11,  13), S(  51,  10), S( 108, -42), S( -52, -14),
    S(  10, -45), S( 183, -44), S( 173, -23), S(  11, -41),
    S( -19, -34), S( 138, -39), S( 138, -39), S( -28, -51),
    S(   1,-131), S(  19, -31), S(  25, -38), S(  26, -67),
    S( -38, -60), S(  62, -51), S(  19, -33), S( -10,-140),
    S( -49,-198), S(  50,-129), S( -62, -97), S(   3, -53),
    S( -76, -74), S( -27, -90), S(  88,-120), S(-114,-149),
};

/* Pawn Evaluation Terms */

const int PawnCandidatePasser[2][RANK_NB] = {
   {S(   0,   0), S( -13, -17), S( -17,  19), S( -19,  31),
    S( -22,  64), S(  20,  65), S(   0,   0), S(   0,   0)},
   {S(   0,   0), S( -13,  22), S(  -7,  28), S(   2,  55),
    S(  23, 119), S(  52,  80), S(   0,   0), S(   0,   0)},
};

const int PawnIsolated[FILE_NB] = {
    S( -15, -10), S(  -1, -15), S(   0, -17), S(   3, -17),
    S(   6, -18), S(   3, -15), S(  -5, -14), S(  -5, -18),
};

const int PawnStacked[2][FILE_NB] = {
   {S(   9, -30), S(  -2, -29), S(  -2, -22), S(  -2, -19),
    S(   1, -19), S(   4, -26), S(   4, -32), S(   7, -30)},
   {S(   4, -17), S(   1, -17), S(  -6, -10), S(  -7, -10),
    S(  -5,  -9), S(  -1, -10), S(   0, -13), S(   1, -19)},
};

const int PawnBackwards[2][RANK_NB] = {
   {S(   0,   0), S(   0,  -8), S(   8,  -7), S(   6, -19),
    S(  -3, -31), S(   0,   0), S(   0,   0), S(   0,   0)},
   {S(   0,   0), S(  -9, -33), S(  -5, -31), S(   3, -31),
    S(  30, -41), S(   0,   0), S(   0,   0), S(   0,   0)},
};

const int PawnConnected32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(  -1, -12), S(  13,  -5), S(   0,  -2), S(   6,   9),
    S(  15,   0), S(  21,  -5), S(  20,   3), S(  18,   9),
    S(   7,  -1), S(  21,   0), S(   6,   4), S(  14,  11),
    S(   9,  14), S(  22,  18), S(  33,  24), S(  26,  19),
    S(  51,  35), S(  40,  64), S(  63,  74), S(  68,  88),
    S( 111,  27), S( 225,  33), S( 204,  77), S( 213,  69),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

/* Knight Evaluation Terms */

const int KnightOutpost[2][2] = {
   {S(  13, -35), S(  42,  -1)},
   {S(   8, -26), S(  22,  -3)},
};

const int KnightBehindPawn = S(   3,  29);

const int KnightInSiberia[4] = {
    S(  -8,  -6), S( -11, -20), S( -26, -21), S( -45, -21),
};

const int KnightMobility[9] = {
    S(-102,-150), S( -43,-117), S( -19, -37), S(  -4,   5),
    S(  10,  17), S(  15,  37), S(  24,  42), S(  34,  42),
    S(  48,  21),
};

/* Bishop Evaluation Terms */

const int BishopPair = S(  22,  90);

const int BishopRammedPawns = S(  -9, -18);

const int BishopOutpost[2][2] = {
   {S(  17, -19), S(  52,  -4)},
   {S(  10, -11), S(  -5,  -3)},
};

const int BishopBehindPawn = S(   4,  24);

const int BishopLongDiagonal = S(  27,  20);

const int BishopMobility[14] = {
    S( -96,-191), S( -42,-124), S( -11, -52), S(   1, -12),
    S(  11,   5), S(  19,  25), S(  22,  39), S(  24,  43),
    S(  24,  53), S(  33,  53), S(  32,  53), S(  57,  37),
    S(  61,  52), S(  88,   8),
};

/* Rook Evaluation Terms */

const int RookFile[2] = { S(   9,   9), S(  35,   8) };

const int RookOnSeventh = S(   2,  41);

const int RookMobility[15] = {
    S(-126,-191), S( -57,-131), S( -25, -86), S( -12, -28),
    S( -10,   4), S( -12,  29), S( -10,  44), S(  -4,  49),
    S(   5,  55), S(  10,  58), S(  12,  67), S(  20,  71),
    S(  21,  76), S(  40,  61), S( 103,  15),
};

/* Queen Evaluation Terms */

const int QueenRelativePin = S( -22, -14);

const int QueenMobility[28] = {
    S(-165,-307), S(-297,-446), S(-130,-255), S( -45,-254),
    S( -19,-179), S(  -8, -84), S(   0, -30), S(   2,   6),
    S(   9,  15), S(  10,  39), S(  16,  45), S(  18,  64),
    S(  21,  54), S(  24,  66), S(  23,  66), S(  22,  72),
    S(  24,  70), S(  16,  73), S(  13,  71), S(  19,  55),
    S(  25,  37), S(  41,  12), S(  36,  -7), S(  29, -24),
    S(   5, -34), S(  -5, -63), S( -27, -22), S(  -2, -33),
};

/* King Evaluation Terms */

const int KingDefenders[12] = {
    S( -40,  -2), S( -20,   2), S(  -1,   6), S(  11,   7),
    S(  21,   7), S(  33,  -1), S(  40, -16), S(   8, -26),
    S(  11,   6), S(  12,   6), S(  12,   6), S(  12,   6),
};

const int KingPawnFileProximity[FILE_NB] = {
    S(  37,  45), S(  21,  30), S(  11,  15), S( -13, -20),
    S(  -7, -60), S(  -4, -73), S(  -7, -81), S(   4, -73),
};

const int KingShelter[2][FILE_NB][RANK_NB] = {
  {{S(  -3,  -7), S(  16, -33), S(  26,  -4), S(  25,   7),
    S(   4,   1), S(  -6,   0), S( -22, -30), S( -59,  26)},
   {S(  14,  -9), S(   2, -17), S(  -6,  -4), S(   4,  -7),
    S( -10,   3), S( -48,  71), S(  77, 102), S( -21,   2)},
   {S(  44,  -4), S(   5,  -6), S( -35,   6), S( -17, -15),
    S(  -7,  -9), S( -22,  10), S(  34,  69), S( -15,   0)},
   {S(  19,  12), S(  25, -18), S(  -1, -14), S(  10, -22),
    S(  22, -34), S( -46,   6), S(-126,  43), S(  -4,  -5)},
   {S( -10,  15), S(   1,  -1), S( -46,   8), S( -29,  12),
    S( -26,   1), S( -40,  -1), S(  60, -35), S( -11,   4)},
   {S(  55, -14), S(  14, -12), S( -26,   7), S( -12, -20),
    S(  10, -34), S(  39, -23), S(  50, -47), S( -22,   2)},
   {S(  43, -18), S(   1, -23), S( -34,   1), S( -27,  -6),
    S( -34,   5), S( -16,  29), S(  18,  46), S( -16,   2)},
   {S(  11, -21), S(   3, -24), S(   9,   3), S(   1,  18),
    S( -11,  26), S(  -7,  48), S(-173,  85), S( -16,  17)}},
  {{S(   0,   0), S( -19, -40), S(   6, -31), S( -56,  15),
    S( -43,   5), S(  -8,  49), S(-177, -14), S( -59,  20)},
   {S(   0,   0), S(  17, -20), S(   9, -13), S( -11,  -9),
    S(   0, -33), S(  28,  70), S(-203,  19), S( -34,  12)},
   {S(   0,   0), S(  20,  -9), S(   3, -12), S(  10, -28),
    S(  31, -11), S( -83,  58), S( -92, -76), S(  -7,   2)},
   {S(   0,   0), S(  -1,   3), S(  -6,  -6), S( -37,  10),
    S( -53,  16), S(-103,  31), S( -67, -55), S( -33,  -7)},
   {S(   0,   0), S(  11,  -2), S(  16, -14), S(  17, -14),
    S(  -6, -12), S( -31,   5), S(-114, -38), S( -18,  -2)},
   {S(   0,   0), S(  -6,  -4), S( -22,   3), S( -15,  -5),
    S(  27, -21), S( -16,   7), S(  37,  46), S( -11,  -2)},
   {S(   0,   0), S(  32, -23), S(  18, -15), S( -11,  -5),
    S( -32,  15), S(   1,  15), S( -39, -40), S( -28,  17)},
   {S(   0,   0), S(  14, -57), S(  15, -32), S( -21,  -6),
    S( -36,  29), S(  -5,  25), S(-230, -38), S( -29,   4)}},
};

const int KingStorm[2][FILE_NB/2][RANK_NB] = {
  {{S( -12,  42), S( 148,  -2), S( -12,  28), S(  -6,   2),
    S( -13,  -2), S( -10,  -6), S( -21,   9), S( -29,  -3)},
   {S( -16,  63), S(  66,  18), S(  -9,  21), S(   8,  11),
    S(   2,   9), S(   5,  -3), S(  -6,   1), S( -17,   8)},
   {S(   1,  52), S(  12,  32), S( -16,  20), S( -13,   9),
    S(  -1,   5), S(   8,   2), S(   8,  -8), S(   7,   8)},
   {S(  -2,  26), S(  16,  24), S( -30,  10), S( -23,   1),
    S( -15,   4), S(  13, -10), S(   2,  -6), S( -20,   8)}},
  {{S(   0,   0), S( -17, -22), S( -19,  -2), S(  29, -27),
    S(  11,  -6), S(  16, -26), S(  -5,   5), S(   4,  30)},
   {S(   0,   0), S(  -7, -48), S(  -2, -19), S(  54, -20),
    S(  16,  -7), S(  22, -30), S(  -8, -20), S( -48,   7)},
   {S(   0,   0), S( -32, -76), S( -16, -15), S(  10,  -9),
    S(   6,  -3), S(  -2, -18), S(  -1, -23), S(  -6,   3)},
   {S(   0,   0), S(   6, -33), S( -29, -20), S( -21,   3),
    S( -10,  -6), S(   9, -37), S(  69, -27), S(   5,  28)}},
};

/* Safety Evaluation Terms */

const int SafetyKnightWeight    = S(  60,  62);
const int SafetyBishopWeight    = S(  33,  35);
const int SafetyRookWeight      = S(  37,  17);
const int SafetyQueenWeight     = S(  38, -41);

const int SafetyAttackValue     = S(  45,  57);
const int SafetyWeakSquares     = S(  43,  45);
const int SafetyNoEnemyQueens   = S(-248,-269);
const int SafetySafeQueenCheck  = S(  89, 135);
const int SafetySafeRookCheck   = S(  85, 148);
const int SafetySafeBishopCheck = S(  59, 180);
const int SafetySafeKnightCheck = S( 111,  91);
const int SafetyAdjustment      = S( -77, -26);
const int SafetyShelter[2][RANK_NB] = {
   {S(   3, -20), S(  -4,  54), S(  -3,  12), S(   2,  30),
    S(   7,  39), S(   6,  24), S(  -2,   4), S(   6, -44)},
   {S(   0,   0), S(  -5,  17), S(  -4,  17), S(   1,  67),
    S(  -3,  61), S(   4,  15), S(  -7,   5), S(   5, -84)},
};

const int SafetyStorm[2][RANK_NB] = {
   {S(   4,  23), S(  -8,  95), S(   0,  56), S(   3,  17),
    S(   2,  37), S(  -3,  24), S(  -3, -25), S(   5, -59)},
   {S(   0,   0), S(   1,   7), S(  -3,  14), S(   2, -50),
    S(   1,  53), S(   0,  74), S(  12,  12), S(  10,  12)},
};


/* Passed Pawn Evaluation Terms */

const int PassedPawn[2][2][RANK_NB] = {
  {{S(   0,   0), S( -31, -12), S( -36,  20), S( -59,  26),
    S(  12,  19), S(  85,  -2), S( 160,  46), S(   0,   0)},
   {S(   0,   0), S( -21,   7), S( -32,  36), S( -52,  41),
    S(   2,  55), S( 102,  60), S( 185, 107), S(   0,   0)}},
  {{S(   0,   0), S( -21,  24), S( -38,  30), S( -56,  53),
    S(  12,  65), S(  95,  80), S( 257, 130), S(   0,   0)},
   {S(   0,   0), S( -22,  19), S( -32,  30), S( -50,  58),
    S(  14,  89), S(  84, 178), S( 113, 320), S(   0,   0)}},
};

const int PassedFriendlyDistance[FILE_NB] = {
    S(   0,   0), S(  -3,   2), S(   1,  -4), S(   6, -14),
    S(   5, -19), S(  -8, -18), S( -10,  -7), S(   0,   0),
};

const int PassedEnemyDistance[FILE_NB] = {
    S(   0,   0), S(   5,   0), S(   5,   2), S(   8,  12),
    S(   0,  25), S(   3,  37), S(  21,  35), S(   0,   0),
};

const int PassedSafePromotionPath = S( -48,  58);

/* Threat Evaluation Terms */

const int ThreatWeakPawn             = S( -11, -39);
const int ThreatMinorAttackedByPawn  = S( -56, -86);
const int ThreatMinorAttackedByMinor = S( -26, -46);
const int ThreatMinorAttackedByMajor = S( -32, -56);
const int ThreatRookAttackedByLesser = S( -49, -28);
const int ThreatMinorAttackedByKing  = S( -46, -21);
const int ThreatRookAttackedByKing   = S( -33, -18);
const int ThreatQueenAttackedByOne   = S( -51,  -8);
const int ThreatOverloadedPieces     = S(  -7, -17);
const int ThreatByPawnPush           = S(  16,  34);

/* Space Evaluation Terms */

const int SpaceRestrictPiece = S(  -4,  -1);
const int SpaceRestrictEmpty = S(  -4,  -2);
const int SpaceCenterControl = S(   4,   0);

/* Closedness Evaluation Terms */

const int ClosednessKnightAdjustment[9] = {
    S(   2,  14), S(   1,  36), S(  -1,  44), S(   3,  44),
    S(   6,  50), S(   8,  43), S(  10,  40), S(  -2,  60),
    S(   1,  38),
};

const int ClosednessRookAdjustment[9] = {
    S(  63,  48), S(   7,  92), S(  15,  70), S(   6,  60),
    S(   3,  55), S(   8,  36), S(   3,  26), S(  -9,  30),
    S( -26,   8),
};

/* Complexity Evaluation Terms */

const int ComplexityTotalPawns  = S(   0,   8);
const int ComplexityPawnFlanks  = S(   0,  82);
const int ComplexityPawnEndgame = S(   0,  76);
const int ComplexityAdjustment  = S(   0,-157);

/* General Evaluation Terms */

const int Tempo = 20;

#undef S

int evaluateBoard(Thread *thread, Board *board) {

    EvalInfo ei;
    int phase, factor, eval, pkeval, hashed;

    // We can recognize positions we just evaluated
    if (thread->moveStack[thread->height-1] == NULL_MOVE)
        return -thread->evalStack[thread->height-1] + 2 * Tempo;

    // Check for this evaluation being cached already
    if (!TRACE && getCachedEvaluation(thread, board, &hashed))
        return hashed;

    initEvalInfo(thread, board, &ei);
    eval = evaluatePieces(&ei, board);

    pkeval = ei.pkeval[WHITE] - ei.pkeval[BLACK];
    if (ei.pkentry == NULL)
        pkeval += computePKNetwork(board);

    eval += pkeval + board->psqtmat + thread->contempt;
    eval += evaluateEndgames(board);
    eval += evaluateClosedness(&ei, board);
    eval += evaluateComplexity(&ei, board, eval);

    // Calculate the game phase based on remaining material (Fruit Method)
    phase = 4 * popcount(board->pieces[QUEEN ])
          + 2 * popcount(board->pieces[ROOK  ])
          + 1 * popcount(board->pieces[KNIGHT]|board->pieces[BISHOP]);

    // Scale evaluation based on remaining material
    factor = evaluateScaleFactor(board, eval);
    if (TRACE) T.factor = factor;

    // Compute and store an interpolated evaluation from white's POV
    eval = (ScoreMG(eval) * phase
         +  ScoreEG(eval) * (24 - phase) * factor / SCALE_NORMAL) / 24;
    storeCachedEvaluation(thread, board, eval);

    // Store a new Pawn King Entry if we did not have one
    if (!TRACE && ei.pkentry == NULL)
        storeCachedPawnKingEval(thread, board, ei.passedPawns, pkeval, ei.pksafety[WHITE], ei.pksafety[BLACK]);

    // Factor in the Tempo after interpolation and scaling, so that
    // if a null move is made, then we know eval = last_eval + 2 * Tempo
    return Tempo + (board->turn == WHITE ? eval : -eval);
}

int evaluatePieces(EvalInfo *ei, Board *board) {

    int eval;

    eval  =   evaluatePawns(ei, board, WHITE)   - evaluatePawns(ei, board, BLACK);

    // This needs to be done after pawn evaluation but before king safety evaluation
    evaluateKingsPawns(ei, board, WHITE);
    evaluateKingsPawns(ei, board, BLACK);

    eval += evaluateKnights(ei, board, WHITE) - evaluateKnights(ei, board, BLACK);
    eval += evaluateBishops(ei, board, WHITE) - evaluateBishops(ei, board, BLACK);
    eval +=   evaluateRooks(ei, board, WHITE)   - evaluateRooks(ei, board, BLACK);
    eval +=  evaluateQueens(ei, board, WHITE)  - evaluateQueens(ei, board, BLACK);
    eval +=   evaluateKings(ei, board, WHITE)   - evaluateKings(ei, board, BLACK);
    eval +=  evaluatePassed(ei, board, WHITE)  - evaluatePassed(ei, board, BLACK);
    eval += evaluateThreats(ei, board, WHITE) - evaluateThreats(ei, board, BLACK);
    eval +=   evaluateSpace(ei, board, WHITE) -   evaluateSpace(ei, board, BLACK);

    return eval;
}

int evaluatePawns(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const int Forward = (colour == WHITE) ? 8 : -8;

    int sq, flag, eval = 0, pkeval = 0;
    uint64_t pawns, myPawns, tempPawns, enemyPawns, attacks;

    // Store off pawn attacks for king safety and threat computations
    ei->attackedBy2[US]      = ei->pawnAttacks[US] & ei->attacked[US];
    ei->attacked[US]        |= ei->pawnAttacks[US];
    ei->attackedBy[US][PAWN] = ei->pawnAttacks[US];

    // Update King Safety calculations
    attacks = ei->pawnAttacks[US] & ei->kingAreas[THEM];
    ei->kingAttacksCount[THEM] += popcount(attacks);

    // Pawn hash holds the rest of the pawn evaluation
    if (ei->pkentry != NULL) return eval;

    pawns = board->pieces[PAWN];
    myPawns = tempPawns = pawns & board->colours[US];
    enemyPawns = pawns & board->colours[THEM];

    // Evaluate each pawn (but not for being passed)
    while (tempPawns) {

        // Pop off the next pawn
        sq = poplsb(&tempPawns);
        if (TRACE) T.PawnValue[US]++;
        if (TRACE) T.PawnPSQT[relativeSquare(US, sq)][US]++;

        uint64_t neighbors   = myPawns    & adjacentFilesMasks(fileOf(sq));
        uint64_t backup      = myPawns    & passedPawnMasks(THEM, sq);
        uint64_t stoppers    = enemyPawns & passedPawnMasks(US, sq);
        uint64_t threats     = enemyPawns & pawnAttacks(US, sq);
        uint64_t support     = myPawns    & pawnAttacks(THEM, sq);
        uint64_t pushThreats = enemyPawns & pawnAttacks(US, sq + Forward);
        uint64_t pushSupport = myPawns    & pawnAttacks(THEM, sq + Forward);
        uint64_t leftovers   = stoppers ^ threats ^ pushThreats;

        // Save passed pawn information for later evaluation
        if (!stoppers) setBit(&ei->passedPawns, sq);

        // Apply a bonus for pawns which will become passers by advancing a
        // square then exchanging our supporters with the remaining stoppers
        else if (!leftovers && popcount(pushSupport) >= popcount(pushThreats)) {
            flag = popcount(support) >= popcount(threats);
            pkeval += PawnCandidatePasser[flag][relativeRankOf(US, sq)];
            if (TRACE) T.PawnCandidatePasser[flag][relativeRankOf(US, sq)][US]++;
        }

        // Apply a penalty if the pawn is isolated. We consider pawns that
        // are able to capture another pawn to not be isolated, as they may
        // have the potential to deisolate by capturing, or be traded away
        if (!threats && !neighbors) {
            pkeval += PawnIsolated[fileOf(sq)];
            if (TRACE) T.PawnIsolated[fileOf(sq)][US]++;
        }

        // Apply a penalty if the pawn is stacked. We adjust the bonus for when
        // the pawn appears to be a candidate to unstack. This occurs when the
        // pawn is not passed but may capture or be recaptured by our own pawns,
        // and when the pawn may freely advance on a file and then be traded away
        if (several(Files[fileOf(sq)] & myPawns)) {
            flag = (stoppers && (threats || neighbors))
                || (stoppers & ~forwardFileMasks(US, sq));
            pkeval += PawnStacked[flag][fileOf(sq)];
            if (TRACE) T.PawnStacked[flag][fileOf(sq)][US]++;
        }

        // Apply a penalty if the pawn is backward. We follow the usual definition
        // of backwards, but also specify that the pawn is not both isolated and
        // backwards at the same time. We don't give backward pawns a connected bonus
        if (neighbors && pushThreats && !backup) {
            flag = !(Files[fileOf(sq)] & enemyPawns);
            pkeval += PawnBackwards[flag][relativeRankOf(US, sq)];
            if (TRACE) T.PawnBackwards[flag][relativeRankOf(US, sq)][US]++;
        }

        // Apply a bonus if the pawn is connected and not backwards. We consider a
        // pawn to be connected when there is a pawn lever or the pawn is supported
        else if (pawnConnectedMasks(US, sq) & myPawns) {
            pkeval += PawnConnected32[relativeSquare32(US, sq)];
            if (TRACE) T.PawnConnected32[relativeSquare32(US, sq)][US]++;
        }
    }

    ei->pkeval[US] = pkeval; // Save eval for the Pawn Hash

    return eval;
}

int evaluateKnights(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, outside, kingDistance, defended, count, eval = 0;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempKnights = board->pieces[KNIGHT] & board->colours[US  ];

    ei->attackedBy[US][KNIGHT] = 0ull;

    // Evaluate each knight
    while (tempKnights) {

        // Pop off the next knight
        sq = poplsb(&tempKnights);
        if (TRACE) T.KnightValue[US]++;
        if (TRACE) T.KnightPSQT[relativeSquare(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = knightAttacks(sq);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][KNIGHT] |= attacks;

        // Apply a bonus if the knight is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the knight
        if (     testBit(outpostRanksMasks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            outside  = testBit(FILE_A | FILE_H, sq);
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += KnightOutpost[outside][defended];
            if (TRACE) T.KnightOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the knight is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += KnightBehindPawn;
            if (TRACE) T.KnightBehindPawn[US]++;
        }

        // Apply a penalty if the knight is far from both kings
        kingDistance = MIN(distanceBetween(sq, ei->kingSquare[THEM]), distanceBetween(sq, ei->kingSquare[US]));
        if (kingDistance >= 4) {
            eval += KnightInSiberia[kingDistance - 4];
            if (TRACE) T.KnightInSiberia[kingDistance - 4][US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += KnightMobility[count];
        if (TRACE) T.KnightMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
            ei->kingAttacksCount[THEM] += popcount(attacks);
            ei->kingAttackersCount[THEM] += 1;
            ei->kingAttackersWeight[THEM] += SafetyKnightWeight;
            if (TRACE) T.SafetyKnightWeight[THEM]++;
        }
    }

    return eval;
}

int evaluateBishops(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, outside, defended, count, eval = 0;
    uint64_t attacks;

    uint64_t enemyPawns  = board->pieces[PAWN  ] & board->colours[THEM];
    uint64_t tempBishops = board->pieces[BISHOP] & board->colours[US  ];

    ei->attackedBy[US][BISHOP] = 0ull;

    // Apply a bonus for having a pair of bishops
    if ((tempBishops & WHITE_SQUARES) && (tempBishops & BLACK_SQUARES)) {
        eval += BishopPair;
        if (TRACE) T.BishopPair[US]++;
    }

    // Evaluate each bishop
    while (tempBishops) {

        // Pop off the next Bishop
        sq = poplsb(&tempBishops);
        if (TRACE) T.BishopValue[US]++;
        if (TRACE) T.BishopPSQT[relativeSquare(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = bishopAttacks(sq, ei->occupiedMinusBishops[US]);
        ei->attackedBy2[US]        |= attacks & ei->attacked[US];
        ei->attacked[US]           |= attacks;
        ei->attackedBy[US][BISHOP] |= attacks;

        // Apply a penalty for the bishop based on number of rammed pawns
        // of our own colour, which reside on the same shade of square as the bishop
        count = popcount(ei->rammedPawns[US] & squaresOfMatchingColour(sq));
        eval += count * BishopRammedPawns;
        if (TRACE) T.BishopRammedPawns[US] += count;

        // Apply a bonus if the bishop is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the bishop.
        if (     testBit(outpostRanksMasks(US), sq)
            && !(outpostSquareMasks(US, sq) & enemyPawns)) {
            outside  = testBit(FILE_A | FILE_H, sq);
            defended = testBit(ei->pawnAttacks[US], sq);
            eval += BishopOutpost[outside][defended];
            if (TRACE) T.BishopOutpost[outside][defended][US]++;
        }

        // Apply a bonus if the bishop is behind a pawn
        if (testBit(pawnAdvance(board->pieces[PAWN], 0ull, THEM), sq)) {
            eval += BishopBehindPawn;
            if (TRACE) T.BishopBehindPawn[US]++;
        }

        // Apply a bonus when controlling both central squares on a long diagonal
        if (   testBit(LONG_DIAGONALS & ~CENTER_SQUARES, sq)
            && several(bishopAttacks(sq, board->pieces[PAWN]) & CENTER_SQUARES)) {
            eval += BishopLongDiagonal;
            if (TRACE) T.BishopLongDiagonal[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += BishopMobility[count];
        if (TRACE) T.BishopMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
            ei->kingAttacksCount[THEM] += popcount(attacks);
            ei->kingAttackersCount[THEM] += 1;
            ei->kingAttackersWeight[THEM] += SafetyBishopWeight;
            if (TRACE) T.SafetyBishopWeight[THEM]++;
        }
    }

    return eval;
}

int evaluateRooks(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, open, count, eval = 0;
    uint64_t attacks;

    uint64_t myPawns    = board->pieces[PAWN] & board->colours[  US];
    uint64_t enemyPawns = board->pieces[PAWN] & board->colours[THEM];
    uint64_t tempRooks  = board->pieces[ROOK] & board->colours[  US];

    ei->attackedBy[US][ROOK] = 0ull;

    // Evaluate each rook
    while (tempRooks) {

        // Pop off the next rook
        sq = poplsb(&tempRooks);
        if (TRACE) T.RookValue[US]++;
        if (TRACE) T.RookPSQT[relativeSquare(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[US]);
        ei->attackedBy2[US]      |= attacks & ei->attacked[US];
        ei->attacked[US]         |= attacks;
        ei->attackedBy[US][ROOK] |= attacks;

        // Rook is on a semi-open file if there are no pawns of the rook's
        // colour on the file. If there are no pawns at all, it is an open file
        if (!(myPawns & Files[fileOf(sq)])) {
            open = !(enemyPawns & Files[fileOf(sq)]);
            eval += RookFile[open];
            if (TRACE) T.RookFile[open][US]++;
        }

        // Rook gains a bonus for being located on seventh rank relative to its
        // colour so long as the enemy king is on the last two ranks of the board
        if (   relativeRankOf(US, sq) == 6
            && relativeRankOf(US, ei->kingSquare[THEM]) >= 6) {
            eval += RookOnSeventh;
            if (TRACE) T.RookOnSeventh[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the rook
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += RookMobility[count];
        if (TRACE) T.RookMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
            ei->kingAttacksCount[THEM] += popcount(attacks);
            ei->kingAttackersCount[THEM] += 1;
            ei->kingAttackersWeight[THEM] += SafetyRookWeight;
            if (TRACE) T.SafetyRookWeight[THEM]++;
        }
    }

    return eval;
}

int evaluateQueens(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, count, eval = 0;
    uint64_t tempQueens, attacks, occupied;

    tempQueens = board->pieces[QUEEN] & board->colours[US];
    occupied = board->colours[WHITE] | board->colours[BLACK];

    ei->attackedBy[US][QUEEN] = 0ull;

    // Evaluate each queen
    while (tempQueens) {

        // Pop off the next queen
        sq = poplsb(&tempQueens);
        if (TRACE) T.QueenValue[US]++;
        if (TRACE) T.QueenPSQT[relativeSquare(US, sq)][US]++;

        // Compute possible attacks and store off information for king safety
        attacks = queenAttacks(sq, occupied);
        ei->attackedBy2[US]       |= attacks & ei->attacked[US];
        ei->attacked[US]          |= attacks;
        ei->attackedBy[US][QUEEN] |= attacks;

        // Apply a penalty if the Queen is at risk for a discovered attack
        if (discoveredAttacks(board, sq, US)) {
            eval += QueenRelativePin;
            if (TRACE) T.QueenRelativePin[US]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the queen
        count = popcount(ei->mobilityAreas[US] & attacks);
        eval += QueenMobility[count];
        if (TRACE) T.QueenMobility[count][US]++;

        // Update King Safety calculations
        if ((attacks &= ei->kingAreas[THEM] & ~ei->pawnAttacksBy2[THEM])) {
            ei->kingAttacksCount[THEM] += popcount(attacks);
            ei->kingAttackersCount[THEM] += 1;
            ei->kingAttackersWeight[THEM] += SafetyQueenWeight;
            if (TRACE) T.SafetyQueenWeight[THEM]++;
        }
    }

    return eval;
}

int evaluateKingsPawns(EvalInfo *ei, Board *board, int colour) {
    // Skip computations if results are cached in the Pawn King Table
    if (ei->pkentry != NULL) return 0;

    const int US = colour, THEM = !colour;

    int dist, blocked;

    uint64_t myPawns     = board->pieces[PAWN ] & board->colours[  US];
    uint64_t enemyPawns  = board->pieces[PAWN ] & board->colours[THEM];

    int kingSq = ei->kingSquare[US];

    // Evaluate based on the number of files between our King and the nearest
    // file-wise pawn. If there is no pawn, kingPawnFileDistance() returns the
    // same distance for both sides causing this evaluation term to be neutral
    dist = kingPawnFileDistance(board->pieces[PAWN], kingSq);
    ei->pkeval[US] += KingPawnFileProximity[dist];
    if (TRACE) T.KingPawnFileProximity[dist][US]++;

    // Evaluate King Shelter & King Storm threat by looking at the file of our King,
    // as well as the adjacent files. When looking at pawn distances, we will use a
    // distance of 7 to denote a missing pawn, since distance 7 is not possible otherwise.
    for (int file = MAX(0, fileOf(kingSq) - 1); file <= MIN(FILE_NB - 1, fileOf(kingSq) + 1); file++) {

        // Find closest friendly pawn at or above our King on a given file
        uint64_t ours = myPawns & Files[file] & forwardRanksMasks(US, rankOf(kingSq));
        int ourDist = !ours ? 7 : abs(rankOf(kingSq) - rankOf(backmost(US, ours)));

        // Find closest enemy pawn at or above our King on a given file
        uint64_t theirs = enemyPawns & Files[file] & forwardRanksMasks(US, rankOf(kingSq));
        int theirDist = !theirs ? 7 : abs(rankOf(kingSq) - rankOf(backmost(US, theirs)));

        // Evaluate King Shelter using pawn distance. Use separate evaluation
        // depending on the file, and if we are looking at the King's file
        ei->pkeval[US] += KingShelter[file == fileOf(kingSq)][file][ourDist];
        if (TRACE) T.KingShelter[file == fileOf(kingSq)][file][ourDist][US]++;

        // Update the Shelter Safety
        ei->pksafety[US] += SafetyShelter[file == fileOf(kingSq)][ourDist];
        if (TRACE) T.SafetyShelter[file == fileOf(kingSq)][ourDist][US]++;

        // Evaluate King Storm using enemy pawn distance. Use a separate evaluation
        // depending on the file, and if the opponent's pawn is blocked by our own
        blocked = (ourDist != 7 && (ourDist == theirDist - 1));
        ei->pkeval[US] += KingStorm[blocked][mirrorFile(file)][theirDist];
        if (TRACE) T.KingStorm[blocked][mirrorFile(file)][theirDist][US]++;

        // Update the Storm Safety
        ei->pksafety[US] += SafetyStorm[blocked][theirDist];
        if (TRACE) T.SafetyStorm[blocked][theirDist][US]++;
    }

    return 0;
}

int evaluateKings(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int count, safety, mg, eg, eval = 0;

    uint64_t enemyQueens = board->pieces[QUEEN] & board->colours[THEM];

    uint64_t defenders  = (board->pieces[PAWN  ] & board->colours[US])
                        | (board->pieces[KNIGHT] & board->colours[US])
                        | (board->pieces[BISHOP] & board->colours[US]);

    int kingSq = ei->kingSquare[US];
    if (TRACE) T.KingValue[US]++;
    if (TRACE) T.KingPSQT[relativeSquare(US, kingSq)][US]++;

    // Bonus for our pawns and minors sitting within our king area
    count = popcount(defenders & ei->kingAreas[US]);
    eval += KingDefenders[count];
    if (TRACE) T.KingDefenders[count][US]++;

    // Perform King Safety when we have two attackers, or
    // one attacker with a potential for a Queen attacker
    if (ei->kingAttackersCount[US] > 1 - popcount(enemyQueens)) {

        // Weak squares are attacked by the enemy, defended no more
        // than once and only defended by our Queens or our King
        uint64_t weak =   ei->attacked[THEM]
                      &  ~ei->attackedBy2[US]
                      & (~ei->attacked[US] | ei->attackedBy[US][QUEEN] | ei->attackedBy[US][KING]);

        // Usually the King Area is 9 squares. Scale are attack counts to account for
        // when the king is in an open area and expects more attacks, or the opposite
        int scaledAttackCounts = 9.0 * ei->kingAttacksCount[US] / popcount(ei->kingAreas[US]);

        // Safe target squares are defended or are weak and attacked by two.
        // We exclude squares containing pieces which we cannot capture.
        uint64_t safe =  ~board->colours[THEM]
                      & (~ei->attacked[US] | (weak & ei->attackedBy2[THEM]));

        // Find square and piece combinations which would check our King
        uint64_t occupied      = board->colours[WHITE] | board->colours[BLACK];
        uint64_t knightThreats = knightAttacks(kingSq);
        uint64_t bishopThreats = bishopAttacks(kingSq, occupied);
        uint64_t rookThreats   = rookAttacks(kingSq, occupied);
        uint64_t queenThreats  = bishopThreats | rookThreats;

        // Identify if there are pieces which can move to the checking squares safely.
        // We consider forking a Queen to be a safe check, even with our own Queen.
        uint64_t knightChecks = knightThreats & safe & ei->attackedBy[THEM][KNIGHT];
        uint64_t bishopChecks = bishopThreats & safe & ei->attackedBy[THEM][BISHOP];
        uint64_t rookChecks   = rookThreats   & safe & ei->attackedBy[THEM][ROOK  ];
        uint64_t queenChecks  = queenThreats  & safe & ei->attackedBy[THEM][QUEEN ];

        safety  = ei->kingAttackersWeight[US];

        safety += SafetyAttackValue     * scaledAttackCounts
                + SafetyWeakSquares     * popcount(weak & ei->kingAreas[US])
                + SafetyNoEnemyQueens   * !enemyQueens
                + SafetySafeQueenCheck  * popcount(queenChecks)
                + SafetySafeRookCheck   * popcount(rookChecks)
                + SafetySafeBishopCheck * popcount(bishopChecks)
                + SafetySafeKnightCheck * popcount(knightChecks)
                + ei->pksafety[US]
                + SafetyAdjustment;

        if (TRACE) T.SafetyAttackValue[US]     = scaledAttackCounts;
        if (TRACE) T.SafetyWeakSquares[US]     = popcount(weak & ei->kingAreas[US]);
        if (TRACE) T.SafetyNoEnemyQueens[US]   = !enemyQueens;
        if (TRACE) T.SafetySafeQueenCheck[US]  = popcount(queenChecks);
        if (TRACE) T.SafetySafeRookCheck[US]   = popcount(rookChecks);
        if (TRACE) T.SafetySafeBishopCheck[US] = popcount(bishopChecks);
        if (TRACE) T.SafetySafeKnightCheck[US] = popcount(knightChecks);
        if (TRACE) T.SafetyAdjustment[US]      = 1;

        // Convert safety to an MG and EG score
        mg = ScoreMG(safety), eg = ScoreEG(safety);
        eval += MakeScore(-mg * MAX(0, mg) / 720, -MAX(0, eg) / 20);
        if (TRACE) T.safety[US] = safety;
    }

    else if (TRACE) {
        T.SafetyKnightWeight[US] = 0;
        T.SafetyBishopWeight[US] = 0;
        T.SafetyRookWeight[US]   = 0;
        T.SafetyQueenWeight[US]  = 0;

        for (int i=0;i<8;i++) {
            T.SafetyShelter[0][i][US]  = 0;
            T.SafetyShelter[1][i][US]  = 0;
            T.SafetyStorm[0][i][US]    = 0;
            T.SafetyStorm[1][i][US]    = 0;
        }
    }

    return eval;
}

int evaluatePassed(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int sq, rank, dist, flag, canAdvance, safeAdvance, eval = 0;

    uint64_t bitboard;
    uint64_t myPassers = board->colours[US] & ei->passedPawns;
    uint64_t occupied  = board->colours[WHITE] | board->colours[BLACK];
    uint64_t tempPawns = myPassers;

    // Evaluate each passed pawn
    while (tempPawns) {

        // Pop off the next passed Pawn
        sq = poplsb(&tempPawns);
        rank = relativeRankOf(US, sq);
        bitboard = pawnAdvance(1ull << sq, 0ull, US);

        // Evaluate based on rank, ability to advance, and safety
        canAdvance = !(bitboard & occupied);
        safeAdvance = !(bitboard & ei->attacked[THEM]);
        eval += PassedPawn[canAdvance][safeAdvance][rank];
        if (TRACE) T.PassedPawn[canAdvance][safeAdvance][rank][US]++;

        // Short-circuit evaluation for additional passers on a file
        if (several(forwardFileMasks(US, sq) & myPassers)) continue;

        // Evaluate based on distance from our king
        dist = distanceBetween(sq, ei->kingSquare[US]);
        eval += dist * PassedFriendlyDistance[rank];
        if (TRACE) T.PassedFriendlyDistance[rank][US] += dist;

        // Evaluate based on distance from their king
        dist = distanceBetween(sq, ei->kingSquare[THEM]);
        eval += dist * PassedEnemyDistance[rank];
        if (TRACE) T.PassedEnemyDistance[rank][US] += dist;

        // Apply a bonus when the path to promoting is uncontested
        bitboard = forwardRanksMasks(US, rankOf(sq)) & Files[fileOf(sq)];
        flag = !(bitboard & (board->colours[THEM] | ei->attacked[THEM]));
        eval += flag * PassedSafePromotionPath;
        if (TRACE) T.PassedSafePromotionPath[US] += flag;
    }

    return eval;
}

int evaluateThreats(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;
    const uint64_t Rank3Rel = US == WHITE ? RANK_3 : RANK_6;

    int count, eval = 0;

    uint64_t friendly = board->colours[  US];
    uint64_t enemy    = board->colours[THEM];
    uint64_t occupied = friendly | enemy;

    uint64_t pawns   = friendly & board->pieces[PAWN  ];
    uint64_t knights = friendly & board->pieces[KNIGHT];
    uint64_t bishops = friendly & board->pieces[BISHOP];
    uint64_t rooks   = friendly & board->pieces[ROOK  ];
    uint64_t queens  = friendly & board->pieces[QUEEN ];

    uint64_t attacksByPawns  = ei->attackedBy[THEM][PAWN  ];
    uint64_t attacksByMinors = ei->attackedBy[THEM][KNIGHT] | ei->attackedBy[THEM][BISHOP];
    uint64_t attacksByMajors = ei->attackedBy[THEM][ROOK  ] | ei->attackedBy[THEM][QUEEN ];

    // Squares with more attackers, few defenders, and no pawn support
    uint64_t poorlyDefended = (ei->attacked[THEM] & ~ei->attacked[US])
                            | (ei->attackedBy2[THEM] & ~ei->attackedBy2[US] & ~ei->attackedBy[US][PAWN]);

    uint64_t weakMinors = (knights | bishops) & poorlyDefended;

    // A friendly minor or major is overloaded if attacked and defended by exactly one
    uint64_t overloaded = (knights | bishops | rooks | queens)
                        & ei->attacked[  US] & ~ei->attackedBy2[  US]
                        & ei->attacked[THEM] & ~ei->attackedBy2[THEM];

    // Look for enemy non-pawn pieces which we may threaten with a pawn advance.
    // Don't consider pieces we already threaten, pawn moves which would be countered
    // by a pawn capture, and squares which are completely unprotected by our pieces.
    uint64_t pushThreat  = pawnAdvance(pawns, occupied, US);
    pushThreat |= pawnAdvance(pushThreat & ~attacksByPawns & Rank3Rel, occupied, US);
    pushThreat &= ~attacksByPawns & (ei->attacked[US] | ~ei->attacked[THEM]);
    pushThreat  = pawnAttackSpan(pushThreat, enemy & ~ei->attackedBy[US][PAWN], US);

    // Penalty for each of our poorly supported pawns
    count = popcount(pawns & ~attacksByPawns & poorlyDefended);
    eval += count * ThreatWeakPawn;
    if (TRACE) T.ThreatWeakPawn[US] += count;

    // Penalty for pawn threats against our minors
    count = popcount((knights | bishops) & attacksByPawns);
    eval += count * ThreatMinorAttackedByPawn;
    if (TRACE) T.ThreatMinorAttackedByPawn[US] += count;

    // Penalty for any minor threat against minor pieces
    count = popcount((knights | bishops) & attacksByMinors);
    eval += count * ThreatMinorAttackedByMinor;
    if (TRACE) T.ThreatMinorAttackedByMinor[US] += count;

    // Penalty for all major threats against poorly supported minors
    count = popcount(weakMinors & attacksByMajors);
    eval += count * ThreatMinorAttackedByMajor;
    if (TRACE) T.ThreatMinorAttackedByMajor[US] += count;

    // Penalty for pawn and minor threats against our rooks
    count = popcount(rooks & (attacksByPawns | attacksByMinors));
    eval += count * ThreatRookAttackedByLesser;
    if (TRACE) T.ThreatRookAttackedByLesser[US] += count;

    // Penalty for king threats against our poorly defended minors
    count = popcount(weakMinors & ei->attackedBy[THEM][KING]);
    eval += count * ThreatMinorAttackedByKing;
    if (TRACE) T.ThreatMinorAttackedByKing[US] += count;

    // Penalty for king threats against our poorly defended rooks
    count = popcount(rooks & poorlyDefended & ei->attackedBy[THEM][KING]);
    eval += count * ThreatRookAttackedByKing;
    if (TRACE) T.ThreatRookAttackedByKing[US] += count;

    // Penalty for any threat against our queens
    count = popcount(queens & ei->attacked[THEM]);
    eval += count * ThreatQueenAttackedByOne;
    if (TRACE) T.ThreatQueenAttackedByOne[US] += count;

    // Penalty for any overloaded minors or majors
    count = popcount(overloaded);
    eval += count * ThreatOverloadedPieces;
    if (TRACE) T.ThreatOverloadedPieces[US] += count;

    // Bonus for giving threats by safe pawn pushes
    count = popcount(pushThreat);
    eval += count * ThreatByPawnPush;
    if (TRACE) T.ThreatByPawnPush[colour] += count;

    return eval;
}

int evaluateSpace(EvalInfo *ei, Board *board, int colour) {

    const int US = colour, THEM = !colour;

    int count, eval = 0;

    uint64_t friendly = board->colours[  US];
    uint64_t enemy    = board->colours[THEM];

    // Squares we attack with more enemy attackers and no friendly pawn attacks
    uint64_t uncontrolled =   ei->attackedBy2[THEM] & ei->attacked[US]
                           & ~ei->attackedBy2[US  ] & ~ei->attackedBy[US][PAWN];

    // Penalty for restricted piece moves
    count = popcount(uncontrolled & (friendly | enemy));
    eval += count * SpaceRestrictPiece;
    if (TRACE) T.SpaceRestrictPiece[US] += count;

    count = popcount(uncontrolled & ~friendly & ~enemy);
    eval += count * SpaceRestrictEmpty;
    if (TRACE) T.SpaceRestrictEmpty[US] += count;

    // Bonus for uncontested central squares
    // This is mostly relevant in the opening and the early middlegame, while rarely correct
    // in the endgame where one rook or queen could control many uncontested squares.
    // Thus we don't apply this term when below a threshold of minors/majors count.
    if (      popcount(board->pieces[KNIGHT] | board->pieces[BISHOP])
        + 2 * popcount(board->pieces[ROOK  ] | board->pieces[QUEEN ]) > 12) {
        count = popcount(~ei->attacked[THEM] & (ei->attacked[US] | friendly) & CENTER_BIG);
        eval += count * SpaceCenterControl;
        if (TRACE) T.SpaceCenterControl[US] += count;
    }

    return eval;
}

int evaluateClosedness(EvalInfo *ei, Board *board) {

    int closedness, count, eval = 0;

    uint64_t white = board->colours[WHITE];
    uint64_t black = board->colours[BLACK];

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t rooks   = board->pieces[ROOK  ];

    // Compute Closedness factor for this position
    closedness = 1 * popcount(board->pieces[PAWN])
               + 3 * popcount(ei->rammedPawns[WHITE])
               - 4 * openFileCount(board->pieces[PAWN]);
    closedness = MAX(0, MIN(8, closedness / 3));

    // Evaluate Knights based on how Closed the position is
    count = popcount(white & knights) - popcount(black & knights);
    eval += count * ClosednessKnightAdjustment[closedness];
    if (TRACE) T.ClosednessKnightAdjustment[closedness][WHITE] += count;

    // Evaluate Rooks based on how Closed the position is
    count = popcount(white & rooks) - popcount(black & rooks);
    eval += count * ClosednessRookAdjustment[closedness];
    if (TRACE) T.ClosednessRookAdjustment[closedness][WHITE] += count;

    return eval;
}

int evaluateComplexity(EvalInfo *ei, Board *board, int eval) {

    // Adjust endgame evaluation based on features related to how
    // likely the stronger side is to convert the position.
    // More often than not, this is a penalty for drawish positions.

    (void) ei; // Silence compiler warning

    int complexity;
    int eg = ScoreEG(eval);
    int sign = (eg > 0) - (eg < 0);

    int pawnsOnBothFlanks = (board->pieces[PAWN] & LEFT_FLANK )
                         && (board->pieces[PAWN] & RIGHT_FLANK);

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    // Compute the initiative bonus or malus for the attacking side
    complexity =  ComplexityTotalPawns  * popcount(board->pieces[PAWN])
               +  ComplexityPawnFlanks  * pawnsOnBothFlanks
               +  ComplexityPawnEndgame * !(knights | bishops | rooks | queens)
               +  ComplexityAdjustment;

    if (TRACE) T.ComplexityTotalPawns[WHITE]  += popcount(board->pieces[PAWN]);
    if (TRACE) T.ComplexityPawnFlanks[WHITE]  += pawnsOnBothFlanks;
    if (TRACE) T.ComplexityPawnEndgame[WHITE] += !(knights | bishops | rooks | queens);
    if (TRACE) T.ComplexityAdjustment[WHITE]  += 1;

    // Avoid changing which side has the advantage
    int v = sign * MAX(ScoreEG(complexity), -abs(eg));

    if (TRACE) T.eval       = eval;
    if (TRACE) T.complexity = complexity;
    return MakeScore(0, v);
}

int evaluateScaleFactor(Board *board, int eval) {

    // Scale endgames based upon the remaining material. We check
    // for various Opposite Coloured Bishop cases, positions with
    // a lone Queen against multiple minor pieces and/or rooks, and
    // positions with a Lone minor that should not be winnable

    const uint64_t pawns   = board->pieces[PAWN  ];
    const uint64_t knights = board->pieces[KNIGHT];
    const uint64_t bishops = board->pieces[BISHOP];
    const uint64_t rooks   = board->pieces[ROOK  ];
    const uint64_t queens  = board->pieces[QUEEN ];

    const uint64_t minors  = knights | bishops;
    const uint64_t pieces  = knights | bishops | rooks;

    const uint64_t white   = board->colours[WHITE];
    const uint64_t black   = board->colours[BLACK];

    const uint64_t weak    = ScoreEG(eval) < 0 ? white : black;
    const uint64_t strong  = ScoreEG(eval) < 0 ? black : white;


    // Check for opposite coloured bishops
    if (   onlyOne(white & bishops)
        && onlyOne(black & bishops)
        && onlyOne(bishops & WHITE_SQUARES)) {

        // Scale factor for OCB + knights
        if ( !(rooks | queens)
            && onlyOne(white & knights)
            && onlyOne(black & knights))
            return SCALE_OCB_ONE_KNIGHT;

        // Scale factor for OCB + rooks
        if ( !(knights | queens)
            && onlyOne(white & rooks)
            && onlyOne(black & rooks))
            return SCALE_OCB_ONE_ROOK;

        // Scale factor for lone OCB
        if (!(knights | rooks | queens))
            return SCALE_OCB_BISHOPS_ONLY;
    }

    // Lone Queens are weak against multiple pieces
    if (onlyOne(queens) && several(pieces) && pieces == (weak & pieces))
        return SCALE_LONE_QUEEN;

    // Lone Minor vs King + Pawns should never be won
    if ((strong & minors) && popcount(strong) == 2)
        return SCALE_DRAW;

    // Scale up lone pieces with massive pawn advantages
    if (   !queens
        && !several(pieces & white)
        && !several(pieces & black)
        &&  popcount(strong & pawns) - popcount(weak & pawns) > 2)
        return SCALE_LARGE_PAWN_ADV;

    return SCALE_NORMAL;
}

void initEvalInfo(Thread *thread, Board *board, EvalInfo *ei) {

    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];

    uint64_t pawns   = board->pieces[PAWN  ];
    uint64_t bishops = board->pieces[BISHOP] | board->pieces[QUEEN];
    uint64_t rooks   = board->pieces[ROOK  ] | board->pieces[QUEEN];
    uint64_t kings   = board->pieces[KING  ];

    // Save some general information about the pawn structure for later
    ei->pawnAttacks[WHITE]    = pawnAttackSpan(white & pawns, ~0ull, WHITE);
    ei->pawnAttacks[BLACK]    = pawnAttackSpan(black & pawns, ~0ull, BLACK);
    ei->pawnAttacksBy2[WHITE] = pawnAttackDouble(white & pawns, ~0ull, WHITE);
    ei->pawnAttacksBy2[BLACK] = pawnAttackDouble(black & pawns, ~0ull, BLACK);
    ei->rammedPawns[WHITE]    = pawnAdvance(black & pawns, ~(white & pawns), BLACK);
    ei->rammedPawns[BLACK]    = pawnAdvance(white & pawns, ~(black & pawns), WHITE);
    ei->blockedPawns[WHITE]   = pawnAdvance(white | black, ~(white & pawns), BLACK);
    ei->blockedPawns[BLACK]   = pawnAdvance(white | black, ~(black & pawns), WHITE);

    // Compute an area for evaluating our King's safety.
    // The definition of the King Area can be found in masks.c
    ei->kingSquare[WHITE] = getlsb(white & kings);
    ei->kingSquare[BLACK] = getlsb(black & kings);
    ei->kingAreas[WHITE] = kingAreaMasks(WHITE, ei->kingSquare[WHITE]);
    ei->kingAreas[BLACK] = kingAreaMasks(BLACK, ei->kingSquare[BLACK]);

    // Exclude squares attacked by our opponents, our blocked pawns, and our own King
    ei->mobilityAreas[WHITE] = ~(ei->pawnAttacks[BLACK] | (white & kings) | ei->blockedPawns[WHITE]);
    ei->mobilityAreas[BLACK] = ~(ei->pawnAttacks[WHITE] | (black & kings) | ei->blockedPawns[BLACK]);

    // Init part of the attack tables. By doing this step here, evaluatePawns()
    // can start by setting up the attackedBy2 table, since King attacks are resolved
    ei->attacked[WHITE] = ei->attackedBy[WHITE][KING] = kingAttacks(ei->kingSquare[WHITE]);
    ei->attacked[BLACK] = ei->attackedBy[BLACK][KING] = kingAttacks(ei->kingSquare[BLACK]);

    // For mobility, we allow bishops to attack through each other
    ei->occupiedMinusBishops[WHITE] = (white | black) ^ (white & bishops);
    ei->occupiedMinusBishops[BLACK] = (white | black) ^ (black & bishops);

    // For mobility, we allow rooks to attack through each other
    ei->occupiedMinusRooks[WHITE] = (white | black) ^ (white & rooks);
    ei->occupiedMinusRooks[BLACK] = (white | black) ^ (black & rooks);

    // Init all of the King Safety information
    ei->kingAttacksCount[WHITE]    = ei->kingAttacksCount[BLACK]    = 0;
    ei->kingAttackersCount[WHITE]  = ei->kingAttackersCount[BLACK]  = 0;
    ei->kingAttackersWeight[WHITE] = ei->kingAttackersWeight[BLACK] = 0;

    // Try to read a hashed Pawn King Eval. Otherwise, start from scratch
    ei->pkentry         = getCachedPawnKingEval(thread, board);
    ei->passedPawns     = ei->pkentry == NULL ? 0ull : ei->pkentry->passed;
    ei->pkeval[WHITE]   = ei->pkentry == NULL ? 0    : ei->pkentry->eval;
    ei->pkeval[BLACK]   = ei->pkentry == NULL ? 0    : 0;
    ei->pksafety[WHITE] = ei->pkentry == NULL ? 0    : ei->pkentry->safetyw;
    ei->pksafety[BLACK] = ei->pkentry == NULL ? 0    : ei->pkentry->safetyb;
}

void initEval() {

    // Init a normalized 64-length PSQT for the evaluation which
    // combines the Piece Values with the original PSQT Values

    for (int sq = 0; sq < SQUARE_NB; sq++) {

        const int sq1 = relativeSquare(WHITE, sq);
        const int sq2 = relativeSquare(BLACK, sq);

        PSQT[WHITE_PAWN  ][sq] = + PawnValue   +   PawnPSQT[sq1];
        PSQT[WHITE_KNIGHT][sq] = + KnightValue + KnightPSQT[sq1];
        PSQT[WHITE_BISHOP][sq] = + BishopValue + BishopPSQT[sq1];
        PSQT[WHITE_ROOK  ][sq] = + RookValue   +   RookPSQT[sq1];
        PSQT[WHITE_QUEEN ][sq] = + QueenValue  +  QueenPSQT[sq1];
        PSQT[WHITE_KING  ][sq] = + KingValue   +   KingPSQT[sq1];

        PSQT[BLACK_PAWN  ][sq] = - PawnValue   -   PawnPSQT[sq2];
        PSQT[BLACK_KNIGHT][sq] = - KnightValue - KnightPSQT[sq2];
        PSQT[BLACK_BISHOP][sq] = - BishopValue - BishopPSQT[sq2];
        PSQT[BLACK_ROOK  ][sq] = - RookValue   -   RookPSQT[sq2];
        PSQT[BLACK_QUEEN ][sq] = - QueenValue  -  QueenPSQT[sq2];
        PSQT[BLACK_KING  ][sq] = - KingValue   -   KingPSQT[sq2];
    }
}
