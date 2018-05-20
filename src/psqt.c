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
    S( -21,   4), S(  10,   3), S(  -7,   7), S(  -5,  -3),
    S( -23,   2), S(  -4,   0), S(  -7,  -6), S(  -2, -11),
    S( -20,   4), S(  -3,   4), S(   7,  -9), S(   4, -20),
    S(  -9,   7), S(   6,   3), S(   2,  -6), S(   5, -21),
    S(   0,  -2), S(  22,  12), S(  23,  -4), S(  29, -22),
    S( -51, -60), S( -36, -18), S(   4, -28), S(   6, -45),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -20, -45), S(   4, -41), S( -14, -24), S(   6, -15),
    S(   2, -63), S(  10, -15), S(  12, -29), S(  17,  -9),
    S(   4, -24), S(  23, -21), S(  11,  -1), S(  25,   9),
    S(  10,   7), S(  28,   6), S(  28,  29), S(  35,  29),
    S(  29,  11), S(  33,   9), S(  40,  34), S(  45,  34),
    S( -17,  12), S(  35,   7), S(  45,  38), S(  49,  28),
    S( -31, -17), S( -31,   5), S(  45, -32), S(  25,  -7),
    S(-163, -33), S(-101, -33), S(-142, -12), S( -37, -34),
};

const int BishopPSQT32[32] = {
    S(  31, -35), S(  26, -44), S(   0, -12), S(  11, -18),
    S(  37, -34), S(  31, -28), S(  28, -25), S(   8,  -7),
    S(  24, -15), S(  31, -13), S(  22,  -3), S(  19,   0),
    S(  20,  -3), S(  10,  -4), S(   9,  11), S(  32,   6),
    S(  -9,  14), S(  24,   3), S(  12,   6), S(  30,  16),
    S(  -7,   7), S(  12,   0), S(  30,   6), S(  19,  12),
    S( -60,   1), S(  -3, -10), S(  -4,  -7), S( -33,   6),
    S( -42, -15), S( -52,   0), S(-124,   2), S( -96,  10),
};

const int RookPSQT32[32] = {
    S(  -4, -32), S(  -6, -23), S(   2, -18), S(   8, -24),
    S( -36, -31), S(  -9, -36), S(  -2, -28), S(   5, -30),
    S( -20, -18), S(   0, -16), S(  -4, -20), S(  -4, -23),
    S( -15,   0), S(  -7,   2), S(  -3,   2), S(   0,   0),
    S(  -5,   9), S(  -6,   4), S(  23,   4), S(  22,   3),
    S(  -7,   8), S(  19,   1), S(  14,   5), S(  28,   8),
    S(   0,  10), S(  -7,   9), S(  36,  -2), S(  26,   1),
    S(   7,  15), S(  19,  12), S( -14,  23), S(  12,  26),
};

const int QueenPSQT32[32] = {
    S(   3, -57), S(  -9, -36), S(  -5, -30), S(  13, -42),
    S(   6, -60), S(  14, -44), S(  21, -57), S(  13, -17),
    S(   3, -28), S(  22, -19), S(  10,   4), S(   3,   0),
    S(   6,  -5), S(   9,   1), S(  -3,  13), S(  -4,  42),
    S( -13,  10), S( -14,  30), S(  -4,  18), S( -21,  48),
    S( -15,   0), S(   0,  15), S(  -3,  17), S( -10,  37),
    S(  -1,   9), S( -66,  49), S(  20,   3), S( -25,  51),
    S( -12, -25), S(  -3, -30), S(   7, -16), S( -24,  -5),
};

const int KingPSQT32[32] = {
    S(  73, -73), S(  89, -69), S(  40, -28), S(  19, -34),
    S(  69, -39), S(  57, -35), S(  12,   0), S( -20,   5),
    S(  -2, -39), S(  43, -28), S(  20,   0), S( -10,  19),
    S( -51, -37), S(  33, -25), S(  -4,   5), S( -54,  29),
    S( -14, -47), S(  46, -40), S( -12,   6), S( -59,  12),
    S(  33, -35), S(  77, -44), S(  52,  -8), S( -12, -15),
    S(  -3, -49), S(  39, -34), S(  22, -20), S(  11, -19),
    S( -67,-142), S(  92, -66), S( -23, -48), S(  -5, -51),
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
