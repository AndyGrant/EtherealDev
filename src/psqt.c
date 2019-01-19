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

#include "assert.h"
#include "bitboards.h"
#include "evaluate.h"
#include "psqt.h"
#include "types.h"

int PSQT[32][SQUARE_NB];

#define S(mg, eg) MakeScore((mg), (eg))

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S( -14,  10), S(  11,   0), S( -11,   4), S(  -9,  -5),
    S( -16,   4), S(  -8,   1), S(  -6,  -8), S(  -4, -15),
    S( -15,  17), S(  -8,  11), S(  18, -13), S(  13, -27),
    S(  -1,  22), S(   6,  11), S(   4,   0), S(  16, -23),
    S(   1,  30), S(   8,  26), S(  13,   4), S(  31, -17),
    S( -44,   5), S( -36,   8), S(  -1, -16), S(   2, -29),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -39, -47), S(  -6, -41), S( -13, -16), S(   0, -11),
    S(   1, -46), S(   7, -12), S(  -1, -21), S(  10,  -8),
    S(   3, -23), S(  25, -16), S(  14,  -4), S(  23,  13),
    S(  21,   6), S(  25,   8), S(  32,  28), S(  31,  33),
    S(  33,   7), S(  35,  12), S(  44,  38), S(  33,  46),
    S( -25,   9), S(  22,   4), S(  41,  37), S(  43,  33),
    S( -29, -17), S( -33,   7), S(  40, -28), S(  18,   4),
    S(-168, -34), S(-102, -29), S(-155,  -3), S( -38, -23),
};

const int BishopPSQT32[32] = {
    S(  23, -22), S(  21, -26), S( -12,  -9), S(  15, -16),
    S(  34, -29), S(  26, -26), S(  24, -12), S(  10,  -4),
    S(  22, -11), S(  31, -12), S(  18,   1), S(  20,   3),
    S(  10,  -6), S(  20,   1), S(  17,  13), S(  21,  16),
    S( -14,   8), S(  23,   2), S(   4,  16), S(  15,  19),
    S(   1,   4), S(   1,  10), S(  23,   7), S(  21,   8),
    S( -63,   4), S( -10,  -3), S(  -5,  -4), S( -38,   2),
    S( -48,   0), S( -61,   0), S(-125,   5), S(-110,  12),
};

const int RookPSQT32[32] = {
    S(  -1, -35), S( -12, -19), S(   1, -25), S(   8, -35),
    S( -52, -27), S( -12, -32), S( -10, -32), S(  -1, -38),
    S( -18, -20), S(   1, -17), S( -13, -24), S(  -2, -25),
    S( -12,  -1), S(  -6,   7), S(  -5,   1), S(   0,   0),
    S(  -3,  17), S(   2,  19), S(  19,   8), S(  25,   8),
    S( -10,  24), S(  19,  15), S(   9,  17), S(  21,  14),
    S(  -1,  16), S(  -8,  19), S(  28,   0), S(  20,  11),
    S(   3,  26), S(  14,  19), S( -17,  30), S(   3,  28),
};

const int QueenPSQT32[32] = {
    S(   3, -46), S( -13, -29), S(  -6, -24), S(  10, -41),
    S(  10, -45), S(  16, -38), S(  22, -60), S(   9, -15),
    S(   6, -21), S(  24, -17), S(   4,   4), S(   0,   0),
    S(   8,  -4), S(  12,   6), S(  -4,  16), S( -18,  47),
    S(  -2,  10), S(  -8,  35), S( -13,  20), S( -26,  52),
    S( -13,   3), S(  -7,  17), S(  -6,  18), S( -11,  43),
    S(  -2,  13), S( -68,  57), S(  15,   8), S( -22,  63),
    S( -15, -18), S(   3,  -9), S(   8,  -3), S( -15,  11),
};

const int KingPSQT32[32] = {
    S(  80,-107), S(  90, -76), S(  37, -35), S(  19, -44),
    S(  64, -54), S(  48, -46), S(   2,  -7), S( -11,  -2),
    S(   2, -40), S(  42, -33), S(  17,   0), S( -10,  17),
    S( -51, -32), S(  34, -17), S(   2,  19), S( -44,  37),
    S( -18, -17), S(  54,   0), S(   8,  32), S( -29,  41),
    S(  38, -17), S(  83,   0), S(  74,  22), S(  10,  21),
    S(  15, -16), S(  50,  -2), S(  33,   1), S(   7,   1),
    S(  27, -81), S(  84, -66), S( -21, -34), S( -15, -34),
};

#undef S

int relativeSquare32(int s, int c) {
    assert(0 <= c && c < COLOUR_NB);
    assert(0 <= s && s < SQUARE_NB);
    static const int edgeDistance[FILE_NB] = {0, 1, 2, 3, 3, 2, 1, 0};
    return 4 * relativeRankOf(c, s) + edgeDistance[fileOf(s)];
}

void initializePSQT() {

    for (int s = 0; s < SQUARE_NB; s++) {
        const int w32 = relativeSquare32(s, WHITE);
        const int b32 = relativeSquare32(s, BLACK);

        PSQT[WHITE_PAWN  ][s] = +MakeScore(PieceValues[PAWN  ][MG], PieceValues[PAWN  ][EG]) +   PawnPSQT32[w32];
        PSQT[WHITE_KNIGHT][s] = +MakeScore(PieceValues[KNIGHT][MG], PieceValues[KNIGHT][EG]) + KnightPSQT32[w32];
        PSQT[WHITE_BISHOP][s] = +MakeScore(PieceValues[BISHOP][MG], PieceValues[BISHOP][EG]) + BishopPSQT32[w32];
        PSQT[WHITE_ROOK  ][s] = +MakeScore(PieceValues[ROOK  ][MG], PieceValues[ROOK  ][EG]) +   RookPSQT32[w32];
        PSQT[WHITE_QUEEN ][s] = +MakeScore(PieceValues[QUEEN ][MG], PieceValues[QUEEN ][EG]) +  QueenPSQT32[w32];
        PSQT[WHITE_KING  ][s] = +MakeScore(PieceValues[KING  ][MG], PieceValues[KING  ][EG]) +   KingPSQT32[w32];

        PSQT[BLACK_PAWN  ][s] = -MakeScore(PieceValues[PAWN  ][MG], PieceValues[PAWN  ][EG]) -   PawnPSQT32[b32];
        PSQT[BLACK_KNIGHT][s] = -MakeScore(PieceValues[KNIGHT][MG], PieceValues[KNIGHT][EG]) - KnightPSQT32[b32];
        PSQT[BLACK_BISHOP][s] = -MakeScore(PieceValues[BISHOP][MG], PieceValues[BISHOP][EG]) - BishopPSQT32[b32];
        PSQT[BLACK_ROOK  ][s] = -MakeScore(PieceValues[ROOK  ][MG], PieceValues[ROOK  ][EG]) -   RookPSQT32[b32];
        PSQT[BLACK_QUEEN ][s] = -MakeScore(PieceValues[QUEEN ][MG], PieceValues[QUEEN ][EG]) -  QueenPSQT32[b32];
        PSQT[BLACK_KING  ][s] = -MakeScore(PieceValues[KING  ][MG], PieceValues[KING  ][EG]) -   KingPSQT32[b32];
    }
}
