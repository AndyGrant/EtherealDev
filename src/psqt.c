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
    S( -19,   9), S(   5,   3), S( -16,   7), S( -11,  -5),
    S( -20,   3), S( -13,   3), S( -11,  -7), S(  -6, -16),
    S( -15,  12), S(  -9,  11), S(  13, -14), S(  11, -28),
    S(   0,  17), S(   7,  12), S(   4,  -3), S(  17, -23),
    S(   5,  26), S(  13,  27), S(  25,  13), S(  47, -10),
    S( -45, -10), S( -41,   1), S(   0, -12), S(   8, -23),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -39, -45), S(  -6, -45), S( -18, -25), S(  -1, -17),
    S(  -1, -41), S(   6, -12), S(   0, -25), S(  13, -15),
    S(   4, -24), S(  23, -19), S(  15,  -8), S(  26,   8),
    S(  21,   3), S(  28,   9), S(  34,  24), S(  33,  29),
    S(  34,  11), S(  32,  15), S(  44,  35), S(  34,  46),
    S( -21,  12), S(  20,   9), S(  38,  36), S(  43,  35),
    S( -18, -12), S( -25,  10), S(  45, -21), S(  23,   9),
    S(-166, -32), S(-100, -26), S(-150,   3), S( -37, -17),
};

const int BishopPSQT32[32] = {
    S(  21, -22), S(  19, -25), S(  -9, -10), S(  11, -16),
    S(  35, -31), S(  28, -32), S(  25, -20), S(  12, -10),
    S(  20, -14), S(  33, -13), S(  19,  -2), S(  21,   0),
    S(  18,  -7), S(  20,  -1), S(  19,   9), S(  19,  15),
    S(  -8,   8), S(  21,   3), S(   7,  16), S(  14,  24),
    S(   3,   4), S(   2,  11), S(  22,  11), S(  22,  10),
    S( -54,  10), S( -16,   0), S(  -2,   0), S( -34,   6),
    S( -47,   0), S( -60,   1), S(-120,  11), S(-108,  17),
};

const int RookPSQT32[32] = {
    S(  -3, -32), S(  -9, -22), S(   2, -26), S(  10, -30),
    S( -50, -19), S( -13, -32), S(  -8, -33), S(   0, -35),
    S( -23, -16), S(  -3, -14), S( -14, -18), S(  -1, -25),
    S( -12,   0), S(  -4,   7), S(  -4,   2), S(   4,  -2),
    S(   1,  14), S(   8,  14), S(  24,   6), S(  32,   4),
    S(  -7,  22), S(  23,  12), S(   8,  16), S(  26,   9),
    S(   2,  11), S( -10,  15), S(  22,   0), S(  19,   6),
    S(  11,  30), S(  17,  25), S( -14,  34), S(   6,  28),
};

const int QueenPSQT32[32] = {
    S(   4, -45), S( -11, -29), S(  -6, -30), S(  13, -41),
    S(  10, -43), S(  17, -41), S(  23, -64), S(  14, -21),
    S(   7, -19), S(  22, -15), S(   5,   4), S(   5,   1),
    S(   8,   0), S(  15,  10), S(  -1,  15), S( -14,  46),
    S(  -7,  18), S(  -6,  38), S( -15,  16), S( -29,  50),
    S( -15,  10), S( -10,  17), S( -15,  12), S( -19,  34),
    S(  -1,  18), S( -64,  59), S(   4,   3), S( -32,  56),
    S( -13, -12), S(   6,  -6), S(   8,  -2), S( -15,  12),
};

const int KingPSQT32[32] = {
    S(  71, -95), S(  80, -59), S(  23, -18), S(   6, -24),
    S(  64, -46), S(  39, -32), S(  -1,   0), S( -15,   3),
    S(  12, -37), S(  47, -34), S(  26,  -2), S(   0,  14),
    S( -45, -31), S(  40, -28), S(   8,  10), S( -38,  29),
    S( -17, -22), S(  54, -17), S(   9,  19), S( -28,  26),
    S(  37, -21), S(  84,  -6), S(  74,  14), S(  11,  15),
    S(  12, -26), S(  49,  -3), S(  34,   6), S(   8,   4),
    S(  25, -83), S(  84, -64), S( -21, -30), S( -15, -30),
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
