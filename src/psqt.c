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
    S( -22,  10), S(   4,   2), S( -15,   7), S(  -9,  -3),
    S( -22,   2), S( -15,   1), S( -11,  -8), S(  -6, -16),
    S( -18,  12), S( -12,  11), S(  14, -15), S(  13, -27),
    S(  -3,  20), S(   5,  14), S(   2,   0), S(  17, -20),
    S(   1,  36), S(   8,  36), S(  20,  23), S(  51,  -2),
    S( -17, -39), S( -64,  -7), S(   2, -20), S(  38, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -49, -25), S(  -8, -44), S( -18, -30), S(  -2, -20),
    S(  -7, -20), S(   3, -13), S(  -1, -31), S(  10, -19),
    S(   1, -24), S(  21, -24), S(  13, -18), S(  24,  -2),
    S(  18,   3), S(  24,   7), S(  31,  17), S(  30,  22),
    S(  27,  17), S(  27,  12), S(  41,  26), S(  31,  39),
    S( -13,  15), S(   8,  13), S(  32,  28), S(  34,  31),
    S(   6, -10), S(  -3,   3), S(  39, -17), S(  42,   2),
    S(-164, -16), S( -82,  -2), S(-111,  18), S( -31,   1),
};

const int BishopPSQT32[32] = {
    S(  17, -16), S(  16, -18), S( -11,  -8), S(   8, -13),
    S(  33, -30), S(  26, -32), S(  24, -21), S(  10, -11),
    S(  17, -13), S(  31, -14), S(  17,  -4), S(  19,  -2),
    S(  18,  -9), S(  17,  -1), S(  17,   6), S(  18,  11),
    S(  -9,   8), S(  18,   3), S(   6,  13), S(  11,  22),
    S(   2,   4), S(   0,  13), S(  17,  11), S(  21,   9),
    S( -46,  11), S( -34,  10), S(  -1,   4), S( -19,   7),
    S( -40,   1), S( -48,   6), S( -89,  15), S( -93,  23),
};

const int RookPSQT32[32] = {
    S(  -6, -30), S( -12, -20), S(   0, -24), S(   8, -30),
    S( -54, -13), S( -15, -30), S(  -9, -30), S(  -1, -33),
    S( -28, -14), S(  -6, -12), S( -16, -15), S(  -3, -24),
    S( -14,   0), S(  -5,   7), S(  -6,   3), S(   5,  -3),
    S(   1,  12), S(  15,   9), S(  25,   4), S(  36,   0),
    S(  -6,  22), S(  29,  10), S(   3,  20), S(  34,   5),
    S(   3,  10), S( -15,  17), S(   6,   8), S(  19,   7),
    S(  37,  21), S(  26,  24), S(   4,  29), S(  14,  25),
};

const int QueenPSQT32[32] = {
    S(  11, -52), S(  -6, -38), S(   2, -50), S(  16, -45),
    S(  11, -38), S(  25, -57), S(  27, -72), S(  18, -25),
    S(   9, -20), S(  26, -17), S(   8,   5), S(   8,   2),
    S(  11,   0), S(  14,  17), S(   0,  20), S( -16,  66),
    S(  -6,  20), S(  -4,  41), S( -13,  21), S( -29,  73),
    S( -22,  27), S( -13,  19), S( -20,  20), S( -10,  25),
    S(  -7,  25), S( -62,  63), S(  -8,  16), S( -41,  54),
    S( -10,  13), S(  15,   3), S(   7,   4), S(  -8,  16),
};

const int KingPSQT32[32] = {
    S(  41, -84), S(  42, -54), S(  -9, -16), S( -25, -25),
    S(  31, -35), S(  -1, -25), S( -35,   1), S( -49,   3),
    S(  11, -35), S(  21, -32), S(  19,  -8), S(  -5,   6),
    S(   0, -35), S(  82, -39), S(  40,   0), S(  -4,  18),
    S(   1, -17), S(  91, -26), S(  42,  12), S(   0,  21),
    S(  46, -16), S( 115, -12), S(  91,  12), S(  34,  11),
    S(   6, -38), S(  46,  -1), S(  32,  13), S(   8,   6),
    S(  11, -92), S(  76, -48), S( -17,  -7), S( -16,  -6),
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
