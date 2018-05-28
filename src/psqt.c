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
    S( -21,   2), S(  15,   3), S(  -4,   7), S(  -4,   1),
    S( -25,   0), S(  -3,  -1), S(  -6,  -6), S(  -2, -10),
    S( -20,   7), S(  -6,   6), S(   7, -12), S(   6, -24),
    S(  -9,  16), S(   2,  11), S(  -1,  -3), S(   5, -25),
    S(   0,  34), S(  23,  34), S(  24,  10), S(  32, -16),
    S( -49,  16), S( -48,  18), S(   1, -11), S(   2, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -36, -46), S(   5, -37), S(  -5, -17), S(   9, -12),
    S(   7, -44), S(  17, -12), S(  16, -27), S(  21,  -6),
    S(  10, -22), S(  26, -19), S(  15,   0), S(  28,  10),
    S(   9,   1), S(  21,   8), S(  28,  25), S(  33,  32),
    S(  27,   7), S(  26,  13), S(  35,  37), S(  37,  42),
    S( -29,  14), S(  21,  10), S(  34,  36), S(  47,  32),
    S(  -9, -25), S( -22,   7), S(  77, -32), S(  38,  -7),
    S(-167, -31), S(-107, -24), S(-131,  -3), S( -43, -26),
};
const int BishopPSQT32[32] = {
    S(  28, -26), S(  22, -32), S(   0,  -7), S(  19, -15),
    S(  41, -33), S(  36, -25), S(  26, -17), S(  12,  -5),
    S(  26, -17), S(  32, -15), S(  23,   1), S(  17,   6),
    S(  15, -11), S(  11,  -5), S(  10,  12), S(  30,  16),
    S( -15,   8), S(  18,   1), S(   1,  14), S(  22,  22),
    S(   1,   3), S(  -6,   8), S(  27,   6), S(  27,   4),
    S( -68,   4), S(  15,  -6), S(   2, -15), S( -27,  -5),
    S( -38,   1), S( -54,  -5), S(-114,   5), S(-115,   9),
};
const int RookPSQT32[32] = {
    S(  -2, -29), S(  -4, -19), S(   7, -14), S(  14, -19),
    S( -36, -22), S(  -5, -27), S(   2, -19), S(   8, -24),
    S( -20, -15), S(   2, -16), S(  -6, -15), S(   2, -20),
    S( -19,   0), S( -20,   1), S(  -8,   0), S(  -2,   0),
    S(  -9,   9), S( -14,   8), S(   9,   0), S(  15,   2),
    S( -21,  14), S(   4,   7), S(  -4,  11), S(  11,   8),
    S(  -4,  11), S( -17,  14), S(  25,   0), S(  10,   4),
    S(  -2,  16), S(   0,  12), S( -22,  21), S(  -9,  21),
};
const int QueenPSQT32[32] = {
    S(  -1, -49), S(  -7, -36), S(  -3, -26), S(  17, -42),
    S(  10, -49), S(  16, -42), S(  21, -53), S(  15, -15),
    S(   8, -25), S(  24, -19), S(   6,   8), S(   3,   3),
    S(   7,  -5), S(   9,   7), S(  -6,  18), S(  -7,  44),
    S( -11,  14), S( -15,  33), S( -11,  25), S( -27,  57),
    S( -21,   6), S(  -8,  17), S(  -4,  24), S(  -5,  45),
    S(  -3,  12), S( -53,  56), S(  15,  12), S( -44,  59),
    S( -37,   0), S(   5,   0), S(  22,  12), S( -27,  13),
};
const int KingPSQT32[32] = {
    S(  92,-105), S(  88, -79), S(  41, -34), S(  21, -40),
    S(  78, -54), S(  54, -45), S(   4,  -4), S( -24,   3),
    S(  10, -40), S(  41, -32), S(  11,   0), S( -25,  16),
    S( -43, -27), S(  33, -18), S(  -7,  14), S( -59,  35),
    S( -17, -16), S(  53,   0), S(   0,  28), S( -43,  34),
    S(  38, -13), S(  91,   0), S(  66,  19), S(  20,  21),
    S(  11, -11), S(  62,   0), S(  43,  -1), S(  -5,   0),
    S(  84, -83), S( 121, -70), S(  11, -29), S(  -1, -35),
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
