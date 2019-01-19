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
    S( -16,   6), S(  13,   0), S(  -3,   1), S(   0,  -3),
    S( -21,   4), S(  -2,   0), S(  -4,  -5), S(   2, -13),
    S( -17,  13), S(  -5,   6), S(  13,  -9), S(   9, -20),
    S(  -5,  18), S(   3,   7), S(   1,   0), S(   7, -20),
    S(   0,  28), S(   9,  26), S(  14,   3), S(  23, -20),
    S( -43,   7), S( -34,   9), S(  -1, -15), S(   1, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -39, -47), S(  -1, -37), S( -10, -15), S(   3,  -7),
    S(   4, -48), S(   7, -11), S(   6, -22), S(  12,  -2),
    S(   4, -23), S(  23, -18), S(  16,   1), S(  27,  12),
    S(   5,   5), S(  26,   7), S(  33,  31), S(  47,  32),
    S(  28,   5), S(  43,  11), S(  44,  40), S(  50,  43),
    S( -27,   8), S(  24,   4), S(  40,  37), S(  48,  32),
    S( -33, -21), S( -35,   4), S(  40, -32), S(  13,  -1),
    S(-168, -34), S(-102, -30), S(-156,  -6), S( -39, -26),
};

const int BishopPSQT32[32] = {
    S(  25, -22), S(  19, -26), S(  -5,  -9), S(  17, -14),
    S(  33, -29), S(  28, -25), S(  22, -12), S(  10,  -2),
    S(  23, -10), S(  29, -11), S(  19,   0), S(  22,   6),
    S(   8,  -5), S(  18,   0), S(  17,  13), S(  32,  17),
    S( -14,  10), S(  35,   2), S(   3,  16), S(  28,  18),
    S(  -1,   5), S(   0,   7), S(  25,   6), S(  20,   6),
    S( -68,   1), S(  -3,  -3), S(  -8, -10), S( -39,   0),
    S( -48,   0), S( -61,  -1), S(-125,   3), S(-110,  10),
};

const int RookPSQT32[32] = {
    S(   0, -30), S(  -8, -16), S(   4, -16), S(  10, -25),
    S( -39, -25), S(  -6, -28), S(   0, -22), S(   5, -30),
    S( -17, -19), S(   4, -14), S(  -3, -20), S(   0, -21),
    S( -16,   0), S(  -9,   4), S(  -2,   2), S(   0,   3),
    S( -10,  12), S(  -8,  10), S(  16,   5), S(  21,   7),
    S( -15,  14), S(  15,   9), S(  10,  13), S(  18,  12),
    S(  -1,  15), S(  -7,  17), S(  33,   0), S(  20,   8),
    S(   0,  23), S(  11,  13), S( -21,  23), S(   3,  27),
};

const int QueenPSQT32[32] = {
    S(   0, -46), S( -13, -29), S(  -3, -21), S(  11, -40),
    S(   7, -46), S(  14, -37), S(  18, -54), S(   8, -14),
    S(   5, -21), S(  23, -16), S(   5,   5), S(   0,   0),
    S(   4,  -4), S(  10,   4), S(  -5,  15), S(  -4,  45),
    S(  -9,  10), S( -12,  33), S(  -7,  21), S( -21,  51),
    S( -11,   3), S(  -5,  17), S(   0,  20), S(  -9,  45),
    S(  -3,  12), S( -73,  55), S(  21,  10), S( -19,  65),
    S( -19, -21), S(   2, -11), S(   8,  -3), S( -17,   9),
};

const int KingPSQT32[32] = {
    S(  81,-106), S(  92, -81), S(  38, -33), S(  22, -39),
    S(  66, -54), S(  55, -42), S(   5,  -1), S( -16,   5),
    S(   0, -41), S(  43, -29), S(  16,   1), S( -11,  19),
    S( -52, -33), S(  33, -19), S(   1,  17), S( -44,  38),
    S( -18, -18), S(  54,   0), S(   8,  31), S( -29,  38),
    S(  38, -17), S(  83,   0), S(  74,  19), S(   9,  16),
    S(  15, -16), S(  50,  -3), S(  33,   0), S(   7,   0),
    S(  27, -81), S(  84, -66), S( -21, -34), S( -15, -35),
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
