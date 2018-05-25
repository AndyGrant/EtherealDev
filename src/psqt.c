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
#include "piece.h"
#include "psqt.h"

int PSQT[32][SQUARE_NB];

#define S(mg, eg) MakeScore((mg), (eg))

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S( -22,   4), S(  11,   3), S(  -4,   4), S(  -6,  -1),
    S( -24,   0), S(  -6,   0), S(  -8,  -7), S(   0, -11),
    S( -21,   5), S(  -8,   5), S(   2, -11), S(   5, -23),
    S(  -7,  15), S(   5,  11), S(   2,  -6), S(   4, -22),
    S(   9,  34), S(  29,  31), S(  28,  10), S(  33, -14),
    S( -26,   6), S( -27,  12), S(   0, -10), S(  14, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -32, -37), S(   0, -41), S( -13, -20), S(   6, -17),
    S(   2, -36), S(   2, -15), S(  15, -23), S(  18,  -9),
    S(   1, -18), S(  17, -17), S(  14,   0), S(  26,  13),
    S(   8,   4), S(  24,   6), S(  30,  26), S(  35,  35),
    S(  26,  15), S(  34,  15), S(  40,  38), S(  55,  39),
    S( -12,  14), S(  35,  10), S(  34,  38), S(  55,  32),
    S(   3, -13), S(  -3,   2), S(  72, -25), S(  30,  -2),
    S(-138, -32), S( -91, -32), S(-118,   0), S( -26, -18),
};

const int BishopPSQT32[32] = {
    S(  16, -17), S(  21, -16), S(   1, -13), S(  15, -13),
    S(  31, -28), S(  35, -23), S(  28, -14), S(   9,  -3),
    S(  24, -14), S(  31, -10), S(  23,   2), S(  20,   7),
    S(  15,  -6), S(   9,  -2), S(  10,  11), S(  30,  15),
    S( -10,  10), S(  16,   2), S(   9,  15), S(  33,  21),
    S(   7,  11), S(   4,   8), S(  26,  10), S(  26,   2),
    S( -43,   7), S(   6,  -3), S(   0,  -3), S( -28,  -2),
    S( -51, -12), S( -59,  -8), S(-110,   0), S(-103,   9),
};

const int RookPSQT32[32] = {
    S(  -2, -31), S(  -2, -17), S(   5, -12), S(  12, -16),
    S( -35, -19), S(  -3, -27), S(  -1, -21), S(   5, -24),
    S( -21, -18), S(   1, -16), S(  -4, -16), S(  -1, -19),
    S( -26,   0), S( -10,   1), S(  -8,  -1), S(  -9,  -1),
    S( -11,  11), S(  -3,   2), S(  13,   4), S(  16,   1),
    S( -11,  10), S(  15,   6), S(   8,   5), S(  20,   5),
    S(  -3,   4), S(  -7,   9), S(  28,  -4), S(  10,   0),
    S(   0,  20), S(   9,  10), S( -24,  23), S(   0,  20),
};

const int QueenPSQT32[32] = {
    S(  -1, -40), S(  -8, -36), S(  -7, -27), S(  18, -37),
    S(   7, -46), S(  13, -45), S(  21, -50), S(  17, -17),
    S(   9, -24), S(  23, -22), S(   7,   4), S(   5,   5),
    S(   3, -12), S(   6,   0), S(  -4,  17), S(  -8,  38),
    S( -23,  20), S( -15,  33), S(  -7,  17), S( -21,  56),
    S( -18,   8), S( -13,  11), S(  -8,  15), S( -11,  44),
    S(   9,   6), S( -59,  52), S(  10,   5), S( -37,  58),
    S( -42, -58), S( -25, -48), S( -21, -33), S( -53, -23),
};

const int KingPSQT32[32] = {
    S(  80,-101), S(  89, -75), S(  35, -29), S(  14, -31),
    S(  70, -52), S(  67, -44), S(  13,  -4), S( -19,   5),
    S(  -4, -39), S(  48, -34), S(  19,  -1), S( -15,  15),
    S( -61, -35), S(  31, -30), S(   7,  11), S( -52,  27),
    S( -26, -26), S(  57, -17), S(   9,  17), S( -30,  22),
    S(  37, -21), S( 109, -10), S(  90,   8), S(  17,   4),
    S(  20, -15), S(  52, -10), S(  49,  -2), S(  32,  -1),
    S(  33, -55), S(  93, -47), S(   6, -23), S(  -7, -22),
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
