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
    S( -19,   9), S(   5,   2), S( -15,   7), S(  -8,  -2),
    S( -20,   2), S( -13,   2), S(  -8,  -7), S(  -5, -15),
    S( -16,  12), S( -10,  11), S(  15, -14), S(  13, -27),
    S(  -1,  18), S(   5,  12), S(   2,  -1), S(  17, -21),
    S(   0,  34), S(   3,  34), S(  14,  22), S(  42,  -2),
    S( -17, -39), S( -64,  -7), S(   1, -19), S(  37, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -48, -25), S(  -9, -45), S( -17, -30), S(  -2, -21),
    S(  -6, -20), S(   3, -13), S(   0, -30), S(  11, -20),
    S(   1, -24), S(  21, -23), S(  15, -17), S(  24,  -1),
    S(  18,   2), S(  23,   7), S(  30,  17), S(  29,  22),
    S(  27,  16), S(  26,  11), S(  40,  26), S(  29,  38),
    S( -13,  14), S(   7,  13), S(  30,  29), S(  34,  31),
    S(   5, -10), S(  -3,   2), S(  39, -18), S(  40,   1),
    S(-163, -16), S( -82,  -2), S(-113,  17), S( -32,   0),
};

const int BishopPSQT32[32] = {
    S(  16, -17), S(  15, -19), S( -11,  -8), S(   7, -13),
    S(  34, -30), S(  26, -31), S(  24, -20), S(  10, -11),
    S(  16, -13), S(  31, -13), S(  18,  -3), S(  19,  -2),
    S(  17,  -8), S(  16,  -1), S(  16,   6), S(  17,  12),
    S(  -9,   7), S(  17,   3), S(   4,  13), S(  10,  22),
    S(   1,   4), S(   0,  13), S(  16,  11), S(  21,   9),
    S( -45,  10), S( -32,   8), S(   0,   3), S( -19,   5),
    S( -41,   0), S( -49,   5), S( -91,  13), S( -95,  21),
};

const int RookPSQT32[32] = {
    S(  -6, -30), S( -12, -20), S(   0, -24), S(   8, -30),
    S( -54, -14), S( -15, -30), S(  -9, -31), S(  -2, -33),
    S( -27, -14), S(  -7, -12), S( -17, -15), S(  -3, -24),
    S( -13,   0), S(  -4,   6), S(  -7,   2), S(   4,  -3),
    S(   0,  11), S(  14,   9), S(  24,   4), S(  35,   0),
    S(  -6,  22), S(  28,   9), S(   2,  20), S(  32,   5),
    S(   3,   9), S( -14,  16), S(   5,   8), S(  18,   6),
    S(  37,  20), S(  26,  23), S(   3,  28), S(  14,  24),
};

const int QueenPSQT32[32] = {
    S(   8, -52), S(  -7, -38), S(   0, -50), S(  15, -44),
    S(   9, -38), S(  23, -56), S(  26, -72), S(  16, -24),
    S(   8, -20), S(  24, -18), S(   5,   4), S(   6,   2),
    S(   9,   0), S(  12,  16), S(   0,  18), S( -17,  64),
    S(  -7,  21), S(  -5,  40), S( -14,  20), S( -30,  71),
    S( -22,  27), S( -13,  19), S( -20,  18), S( -10,  23),
    S(  -7,  25), S( -62,  62), S(  -8,  15), S( -41,  53),
    S( -14,   9), S(  12,   0), S(   5,   1), S( -10,  14),
};

const int KingPSQT32[32] = {
    S(  39, -82), S(  41, -52), S(  -9, -14), S( -26, -22),
    S(  33, -34), S(   0, -25), S( -33,   1), S( -47,   3),
    S(  13, -35), S(  24, -32), S(  21,  -7), S(  -3,   6),
    S(   0, -36), S(  82, -39), S(  40,   0), S(  -4,  18),
    S(   1, -19), S(  90, -26), S(  40,  12), S(  -1,  21),
    S(  46, -18), S( 113, -12), S(  90,  11), S(  33,  10),
    S(   6, -39), S(  46,  -1), S(  32,  12), S(   8,   6),
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
