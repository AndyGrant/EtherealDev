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
    S( -21,   3), S(  16,   3), S(  -3,   8), S(  -4,   3),
    S( -25,   0), S(  -4,  -1), S(  -6,  -7), S(  -2, -10),
    S( -20,   8), S(  -6,   7), S(   9, -13), S(   7, -25),
    S(  -9,  18), S(   2,  12), S(  -2,  -2), S(   6, -25),
    S(   1,  38), S(  26,  37), S(  28,  13), S(  38, -15),
    S( -49,  21), S( -49,  23), S(   4,  -9), S(  10, -29),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -32, -45), S(   7, -36), S(  -4, -18), S(  12, -14),
    S(   9, -42), S(  20, -13), S(  16, -28), S(  22,  -7),
    S(  12, -24), S(  27, -20), S(  16,  -1), S(  29,   8),
    S(  11,  -1), S(  16,   7), S(  27,  21), S(  31,  30),
    S(  24,   8), S(  25,  13), S(  32,  34), S(  36,  41),
    S( -34,  16), S(  17,  12), S(  32,  33), S(  47,  29),
    S(  -9, -26), S( -24,   7), S(  77, -36), S(  36, -12),
    S(-176, -28), S(-115, -20), S(-136,  -4), S( -49, -27),
};
const int BishopPSQT32[32] = {
    S(  29, -30), S(  23, -35), S(   1,  -7), S(  19, -16),
    S(  42, -36), S(  36, -26), S(  27, -19), S(  13,  -6),
    S(  28, -20), S(  33, -17), S(  23,   0), S(  17,   5),
    S(  16, -14), S(  11,  -7), S(  10,  11), S(  29,  15),
    S( -15,   6), S(  19,   1), S(   0,  14), S(  21,  22),
    S(   1,   1), S( -10,  10), S(  25,   6), S(  28,   3),
    S( -69,   6), S(  14,  -9), S(   2, -17), S( -26,  -8),
    S( -35,   0), S( -51,  -7), S(-109,   3), S(-118,   9),
};
const int RookPSQT32[32] = {
    S(  -1, -28), S(  -3, -20), S(   8, -14), S(  16, -19),
    S( -37, -21), S(  -6, -26), S(   2, -18), S(   9, -22),
    S( -21, -14), S(   2, -17), S(  -7, -13), S(   3, -20),
    S( -19,  -1), S( -21,   1), S(  -8,  -2), S(  -1,  -1),
    S(  -9,   8), S( -16,   8), S(  10,   0), S(  14,   1),
    S( -23,  13), S(   0,   7), S(  -8,  12), S(  13,   5),
    S(  -8,   9), S( -22,  13), S(  18,  -1), S(   6,   2),
    S(  -4,  15), S(  -7,  13), S( -24,  20), S( -11,  19),
};
const int QueenPSQT32[32] = {
    S(   1, -51), S(  -7, -39), S(   0, -28), S(  19, -43),
    S(  11, -49), S(  18, -46), S(  22, -53), S(  17, -14),
    S(   8, -26), S(  24, -20), S(   7,  11), S(   5,   5),
    S(   8,  -6), S(   7,   8), S(  -7,  21), S(  -8,  43),
    S( -16,  15), S( -16,  30), S( -16,  26), S( -31,  60),
    S( -28,   8), S( -12,  17), S( -12,  22), S(  -8,  41),
    S( -11,  13), S( -61,  56), S(   8,  14), S( -51,  58),
    S( -50,  14), S(  -4,   5), S(  15,  19), S( -32,  15),
};
const int KingPSQT32[32] = {
    S(  90,-108), S(  86, -78), S(  39, -33), S(  20, -40),
    S(  77, -56), S(  54, -44), S(  10,  -4), S( -19,   4),
    S(  13, -42), S(  50, -34), S(  22,   0), S( -14,  18),
    S( -39, -28), S(  43, -18), S(   2,  15), S( -49,  35),
    S( -22, -16), S(  58,   0), S(   5,  28), S( -36,  34),
    S(  28, -12), S(  93,  -1), S(  68,  18), S(  27,  21),
    S(  -4, -10), S(  48,   1), S(  46,  -3), S(  -8,   0),
    S(  93, -92), S( 127, -77), S(  14, -32), S(  -4, -38),
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
