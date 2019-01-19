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
    S( -18,   7), S(  10,   0), S(  -9,   4), S(  -4,  -4),
    S( -21,   3), S(  -4,  -1), S(  -7,  -5), S(  -2, -12),
    S( -15,  12), S(  -4,   7), S(  14, -12), S(  11, -22),
    S(  -2,  18), S(   5,   8), S(   4,  -1), S(  12, -20),
    S(   1,  26), S(  10,  25), S(  16,   4), S(  26, -18),
    S( -43,   5), S( -34,   8), S(  -1, -15), S(   1, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -39, -47), S(  -3, -37), S( -11, -16), S(   2,  -8),
    S(   3, -48), S(   7, -10), S(   4, -22), S(  15,  -3),
    S(   5, -22), S(  25, -17), S(  17,   1), S(  29,  14),
    S(  12,   6), S(  27,   7), S(  36,  31), S(  41,  31),
    S(  28,   5), S(  39,  11), S(  44,  39), S(  43,  43),
    S( -27,   8), S(  24,   4), S(  40,  37), S(  48,  33),
    S( -32, -20), S( -35,   5), S(  40, -31), S(  14,   0),
    S(-168, -34), S(-102, -30), S(-156,  -6), S( -39, -26),
};
const int BishopPSQT32[32] = {
    S(  24, -21), S(  19, -26), S(  -5,  -9), S(  16, -15),
    S(  33, -29), S(  30, -25), S(  23, -13), S(  13,  -3),
    S(  23, -11), S(  31, -11), S(  19,   1), S(  22,   6),
    S(  10,  -5), S(  18,   0), S(  18,  13), S(  29,  16),
    S( -13,  10), S(  29,   2), S(   4,  16), S(  26,  19),
    S(   0,   5), S(   0,   7), S(  25,   7), S(  20,   6),
    S( -67,   2), S(  -4,  -3), S(  -8,  -9), S( -39,   1),
    S( -48,   0), S( -61,  -1), S(-125,   3), S(-110,  10),
};
const int RookPSQT32[32] = {
    S(   0, -29), S(  -8, -17), S(   4, -19), S(  12, -25),
    S( -41, -25), S(  -7, -29), S(  -3, -25), S(   4, -31),
    S( -17, -18), S(   3, -14), S(  -4, -20), S(   0, -21),
    S( -15,   0), S(  -8,   5), S(  -1,   2), S(   1,   2),
    S(  -8,  13), S(  -6,  11), S(  17,   6), S(  22,   7),
    S( -14,  16), S(  16,  10), S(  10,  13), S(  18,  12),
    S(  -1,  14), S(  -7,  16), S(  32,   0), S(  20,   7),
    S(   1,  24), S(  11,  14), S( -21,  24), S(   3,  27),
};
const int QueenPSQT32[32] = {
    S(   0, -46), S( -12, -29), S(  -5, -22), S(  12, -39),
    S(   7, -46), S(  14, -37), S(  19, -54), S(  11, -14),
    S(   5, -21), S(  23, -16), S(   4,   5), S(   1,   1),
    S(   6,  -4), S(  11,   4), S(  -4,  15), S(  -9,  44),
    S(  -8,  10), S( -11,  33), S(  -8,  20), S( -23,  51),
    S( -12,   3), S(  -5,  17), S(  -2,  19), S( -10,  44),
    S(  -3,  12), S( -73,  55), S(  20,  10), S( -20,  65),
    S( -19, -21), S(   2, -11), S(   8,  -3), S( -17,   9),
};
const int KingPSQT32[32] = {
    S(  79,-105), S(  91, -78), S(  37, -30), S(  20, -37),
    S(  67, -53), S(  53, -42), S(   5,  -1), S( -14,   5),
    S(   1, -40), S(  43, -29), S(  17,   1), S( -11,  19),
    S( -52, -33), S(  33, -20), S(   1,  15), S( -44,  35),
    S( -18, -19), S(  53,  -1), S(   7,  28), S( -30,  36),
    S(  37, -18), S(  83,  -1), S(  73,  18), S(   9,  15),
    S(  14, -17), S(  50,  -3), S(  33,   0), S(   7,   0),
    S(  26, -81), S(  84, -66), S( -21, -34), S( -15, -35),
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
