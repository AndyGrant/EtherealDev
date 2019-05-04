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
    S( -20,   9), S(   1,   3), S( -17,   8), S( -12,  -3),
    S( -21,   1), S( -12,   1), S( -11,  -6), S( -10, -15),
    S( -13,  12), S(  -6,  13), S(  19, -13), S(  15, -27),
    S(   0,  18), S(   8,  12), S(   4,   0), S(  24, -19),
    S(   0,  33), S(   3,  34), S(  15,  22), S(  42,  -2),
    S( -17, -40), S( -65,  -7), S(   2, -19), S(  36, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -49, -26), S( -11, -46), S( -19, -32), S(  -2, -21),
    S(  -8, -21), S(   2, -14), S(  -2, -32), S(   8, -21),
    S(  -3, -25), S(  19, -24), S(   9, -17), S(  25,   0),
    S(  21,   3), S(  25,   7), S(  31,  17), S(  28,  21),
    S(  27,  17), S(  28,  12), S(  41,  27), S(  29,  39),
    S( -12,  15), S(   8,  13), S(  31,  30), S(  34,  32),
    S(   6,  -9), S(  -3,   2), S(  40, -17), S(  40,   2),
    S(-163, -15), S( -82,  -2), S(-113,  17), S( -32,   0),
};

const int BishopPSQT32[32] = {
    S(  17, -17), S(  16, -20), S( -17,  -8), S(   6, -14),
    S(  34, -30), S(  21, -34), S(  25, -21), S(   8, -11),
    S(  15, -13), S(  31, -14), S(  15,  -4), S(  20,  -2),
    S(  19,  -7), S(  17,   0), S(  17,   7), S(  17,  12),
    S(  -9,   8), S(  17,   3), S(   6,  14), S(  12,  23),
    S(   1,   4), S(   1,  13), S(  17,  13), S(  21,   9),
    S( -44,  10), S( -31,   8), S(   0,   4), S( -19,   6),
    S( -42,   0), S( -49,   5), S( -91,  13), S( -95,  21),
};

const int RookPSQT32[32] = {
    S(  -8, -32), S( -15, -23), S(  -2, -28), S(   6, -33),
    S( -59, -17), S( -17, -32), S( -11, -34), S(  -5, -38),
    S( -28, -14), S(  -9, -12), S( -17, -16), S(  -4, -25),
    S( -11,   0), S(  -3,   7), S(  -6,   3), S(   5,  -2),
    S(   2,  12), S(  16,  11), S(  26,   6), S(  36,   1),
    S(  -4,  24), S(  30,  11), S(   3,  21), S(  34,   7),
    S(   4,   8), S( -13,  15), S(   5,   8), S(  18,   6),
    S(  37,  22), S(  26,  24), S(   3,  28), S(  14,  24),
};

const int QueenPSQT32[32] = {
    S(   8, -52), S(  -7, -38), S(  -3, -51), S(  10, -45),
    S(   9, -38), S(  23, -57), S(  27, -73), S(  14, -26),
    S(   9, -19), S(  24, -17), S(   7,   3), S(   6,   0),
    S(   9,   0), S(  14,  16), S(   0,  18), S( -21,  64),
    S(  -6,  21), S(  -5,  40), S( -13,  20), S( -31,  71),
    S( -22,  27), S( -13,  19), S( -19,  18), S( -10,  23),
    S(  -6,  26), S( -63,  62), S(  -8,  15), S( -41,  53),
    S( -14,   9), S(  12,   0), S(   5,   1), S( -10,  14),
};

const int KingPSQT32[32] = {
    S(  40, -83), S(  45, -51), S( -10, -13), S( -27, -23),
    S(  31, -35), S(  -2, -24), S( -34,   4), S( -47,   4),
    S(  12, -35), S(  23, -30), S(  21,  -5), S(  -2,   7),
    S(   0, -35), S(  82, -39), S(  40,   0), S(  -4,  19),
    S(   1, -19), S(  90, -26), S(  40,  11), S(  -1,  20),
    S(  45, -18), S( 113, -14), S(  89,   9), S(  32,   8),
    S(   5, -41), S(  45,  -2), S(  32,  11), S(   8,   5),
    S(  11, -92), S(  77, -49), S( -17,  -6), S( -16,  -6),
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
