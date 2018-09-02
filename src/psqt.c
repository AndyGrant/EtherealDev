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
    S(  -7,  18), S(  15,   6), S(  -4,  10), S(  -2,   0),
    S( -10,  10), S(  -5,   7), S(  -1,  -3), S(   2, -11),
    S(  -9,  24), S(  -4,  18), S(  24,  -7), S(  18, -21),
    S(   4,  31), S(  11,  20), S(   8,   7), S(  22, -16),
    S(   6,  40), S(  12,  33), S(  17,   9), S(  32, -18),
    S( -43,  15), S( -36,  13), S(  -2, -14), S(   2, -29),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -40, -48), S( -11, -42), S( -16, -18), S(  -3, -15),
    S(   0, -46), S(   6, -13), S(  -6, -23), S(   3, -10),
    S(  -7, -23), S(  18, -16), S(   6,  -7), S(  15,  13),
    S(  18,   4), S(  22,   9), S(  24,  27), S(  23,  35),
    S(  35,   9), S(  28,  13), S(  38,  37), S(  27,  46),
    S( -24,  11), S(  24,   5), S(  40,  38), S(  42,  35),
    S( -26, -16), S( -32,   9), S(  43, -25), S(  22,   8),
    S(-168, -34), S(-101, -28), S(-156,  -3), S( -39, -21),
};
const int BishopPSQT32[32] = {
    S(  22, -24), S(  21, -28), S( -17, -10), S(  10, -18),
    S(  32, -31), S(  22, -29), S(  21, -15), S(   4,  -7),
    S(  21, -13), S(  26, -14), S(  15,   0), S(  16,   0),
    S(  16,  -9), S(  20,   1), S(  15,  11), S(  24,  15),
    S( -14,   8), S(  32,   0), S(   5,  16), S(  17,  21),
    S(  -1,   3), S(   0,  10), S(  24,   8), S(  23,   9),
    S( -63,   6), S( -13,  -3), S(  -3,  -4), S( -39,   3),
    S( -50,   1), S( -63,  -1), S(-126,   8), S(-112,  14),
};
const int RookPSQT32[32] = {
    S(  -2, -39), S( -13, -24), S(   0, -28), S(   6, -38),
    S( -55, -28), S( -16, -35), S( -10, -34), S(  -4, -40),
    S( -21, -22), S(  -1, -19), S( -15, -24), S(  -6, -27),
    S( -14,  -3), S(  -7,   8), S(  -6,   1), S(  -2,   0),
    S(  -3,  17), S(   4,  21), S(  21,   8), S(  25,   8),
    S( -11,  26), S(  20,  16), S(  10,  18), S(  21,  14),
    S(  -2,  16), S( -11,  20), S(  27,   0), S(  19,  11),
    S(   5,  24), S(  15,  19), S( -16,  32), S(   3,  27),
};
const int QueenPSQT32[32] = {
    S(   4, -46), S( -14, -30), S(  -7, -26), S(   9, -44),
    S(  12, -46), S(  15, -40), S(  21, -63), S(   8, -20),
    S(   7, -22), S(  22, -21), S(   3,   4), S(  -2,  -1),
    S(   6,  -5), S(   9,   6), S(  -4,  19), S( -16,  47),
    S(  -1,   9), S(  -9,  36), S( -14,  22), S( -28,  55),
    S( -13,   5), S(  -7,  19), S(  -9,  20), S( -11,  45),
    S(  -2,  14), S( -66,  59), S(  14,   9), S( -24,  65),
    S( -14, -17), S(   5,  -9), S(  10,  -3), S( -14,  13),
};
const int KingPSQT32[32] = {
    S(  83,-108), S(  90, -77), S(  37, -38), S(  20, -48),
    S(  63, -54), S(  47, -47), S(   3, -11), S( -13,  -6),
    S(   0, -41), S(  42, -35), S(  17,  -3), S( -12,  14),
    S( -51, -31), S(  34, -17), S(   3,  18), S( -46,  34),
    S( -18, -15), S(  56,   1), S(   8,  33), S( -31,  42),
    S(  40, -15), S(  85,   2), S(  75,  25), S(  10,  25),
    S(  17, -15), S(  52,  -1), S(  35,   3), S(   9,   3),
    S(  29, -81), S(  86, -66), S( -21, -34), S( -15, -34),
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
