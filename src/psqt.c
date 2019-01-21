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
    S( -20,  10), S(   6,   4), S( -14,   8), S(  -8,  -3),
    S( -20,   3), S( -13,   4), S(  -9,  -7), S(  -4, -16),
    S( -16,  12), S(  -9,  13), S(  15, -14), S(  14, -28),
    S(  -1,  18), S(   6,  14), S(   4,  -1), S(  17, -21),
    S(   0,  34), S(   3,  39), S(  13,  28), S(  39,   1),
    S(  -5, -50), S( -63,  -8), S(   8, -20), S(  43, -34),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -52, -23), S(  -9, -48), S( -19, -32), S(  -3, -23),
    S(  -8, -22), S(   2, -15), S(  -2, -33), S(   9, -22),
    S(   1, -27), S(  20, -27), S(  12, -19), S(  24,  -3),
    S(  17,   4), S(  22,   7), S(  30,  17), S(  28,  22),
    S(  26,  18), S(  27,  11), S(  39,  28), S(  29,  40),
    S( -13,  16), S(   7,  14), S(  30,  31), S(  32,  33),
    S(   9, -13), S(  -4,   1), S(  41, -20), S(  45,  -1),
    S(-165, -13), S( -76,  -5), S(-104,  13), S( -34,  -1),
};

const int BishopPSQT32[32] = {
    S(  17, -18), S(  15, -22), S( -11, -11), S(   8, -15),
    S(  33, -33), S(  26, -34), S(  24, -23), S(  10, -13),
    S(  18, -15), S(  32, -15), S(  17,  -5), S(  19,  -3),
    S(  17,  -9), S(  17,  -2), S(  17,   6), S(  16,  12),
    S(  -9,   8), S(  18,   3), S(   4,  14), S(  10,  23),
    S(   2,   5), S(  -1,  13), S(  15,  13), S(  20,  10),
    S( -45,  10), S( -35,   9), S(  -3,   3), S( -24,   4),
    S( -44,  -1), S( -48,   3), S( -86,  11), S( -94,  19),
};

const int RookPSQT32[32] = {
    S(  -7, -31), S( -13, -21), S(   0, -25), S(   6, -29),
    S( -54, -16), S( -16, -31), S( -11, -32), S(  -3, -34),
    S( -28, -15), S(  -8, -12), S( -18, -16), S(  -4, -25),
    S( -15,   0), S(  -6,   6), S(  -8,   2), S(   3,  -4),
    S(   0,  11), S(  14,   8), S(  23,   4), S(  35,   0),
    S(  -9,  21), S(  28,   8), S(   0,  18), S(  30,   4),
    S(   6,   8), S( -12,  15), S(   4,   7), S(  19,   5),
    S(  39,  19), S(  27,  22), S(   5,  27), S(  15,  24),
};

const int QueenPSQT32[32] = {
    S(   8, -57), S(  -9, -40), S(   0, -54), S(  14, -47),
    S(   9, -41), S(  22, -59), S(  25, -74), S(  15, -27),
    S(   7, -23), S(  23, -20), S(   4,   4), S(   4,   3),
    S(   9,  -4), S(  12,  14), S(  -2,  17), S( -19,  66),
    S(  -9,  21), S(  -7,  38), S( -16,  19), S( -36,  74),
    S( -23,  25), S( -16,  18), S( -23,  19), S( -13,  22),
    S(  -4,  24), S( -60,  59), S( -13,  16), S( -45,  54),
    S( -20,   7), S(  12,  -7), S(   3,  -1), S( -12,  10),
};

const int KingPSQT32[32] = {
    S(  34, -75), S(  32, -47), S( -18, -11), S( -33, -18),
    S(  28, -29), S(  -8, -22), S( -40,   3), S( -52,   7),
    S(   9, -33), S(  17, -32), S(  18,  -9), S(  -4,   8),
    S(   6, -38), S(  83, -42), S(  43,  -4), S(  -5,  17),
    S(  14, -23), S( 106, -32), S(  56,   4), S(   8,  15),
    S(  54, -20), S( 136, -20), S( 103,   4), S(  51,   4),
    S(   5, -41), S(  41,  -3), S(  29,  11), S(   6,   7),
    S(   0, -88), S(  68, -46), S( -23,  -6), S( -25,  -2),
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
