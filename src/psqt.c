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
    S( -22,   2), S(  14,   3), S(  -6,   6), S(  -4,   1),
    S( -26,   0), S(  -4,  -1), S(  -6,  -7), S(  -2, -11),
    S( -21,   7), S(  -7,   6), S(   6, -12), S(   5, -24),
    S(  -8,  16), S(   3,  11), S(  -1,  -2), S(   5, -24),
    S(   4,  34), S(  25,  33), S(  23,  11), S(  33, -16),
    S( -43,  15), S( -43,  18), S(   2, -12), S(   1, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -37, -46), S(   4, -37), S(  -8, -16), S(   8, -11),
    S(   6, -45), S(  15, -11), S(  15, -27), S(  20,  -5),
    S(   7, -23), S(  25, -18), S(  15,   1), S(  27,  11),
    S(  10,   2), S(  26,   8), S(  29,  26), S(  35,  32),
    S(  32,   8), S(  30,  13), S(  39,  37), S(  44,  41),
    S( -25,  14), S(  26,  11), S(  40,  36), S(  53,  33),
    S(  -7, -23), S( -21,   8), S(  82, -31), S(  41,  -6),
    S(-167, -31), S(-107, -24), S(-131,  -2), S( -42, -26),
};

const int BishopPSQT32[32] = {
    S(  29, -25), S(  25, -30), S(   0,  -7), S(  17, -15),
    S(  41, -32), S(  35, -24), S(  28, -17), S(  12,  -4),
    S(  28, -16), S(  31, -14), S(  23,   1), S(  18,   7),
    S(  15,  -9), S(  11,  -4), S(  11,  13), S(  34,  17),
    S( -13,   9), S(  21,   2), S(   6,  15), S(  27,  21),
    S(   3,   4), S(  -4,   8), S(  28,   7), S(  27,   4),
    S( -59,   4), S(  15,  -5), S(   3, -14), S( -27,  -4),
    S( -38,   1), S( -58,  -4), S(-114,   6), S(-113,  10),
};

const int RookPSQT32[32] = {
    S(  -2, -30), S(  -4, -19), S(   7, -14), S(  13, -19),
    S( -37, -23), S(  -6, -27), S(   2, -18), S(   7, -24),
    S( -20, -16), S(   4, -16), S(  -6, -14), S(   1, -19),
    S( -19,   0), S( -17,   2), S(  -8,   0), S(  -3,   1),
    S(  -9,  10), S( -13,   9), S(  11,   1), S(  14,   3),
    S( -17,  15), S(   9,   8), S(   0,  12), S(  18,   9),
    S(  -3,  12), S( -13,  15), S(  29,   0), S(  13,   5),
    S(   0,  17), S(   3,  12), S( -21,  22), S(  -5,  22),
};

const int QueenPSQT32[32] = {
    S(  -1, -50), S(  -7, -35), S(  -1, -25), S(  18, -41),
    S(   9, -48), S(  17, -42), S(  23, -52), S(  17, -14),
    S(   7, -25), S(  24, -17), S(   7,   8), S(   5,   5),
    S(   9,  -9), S(   9,   6), S(  -7,  17), S(  -7,  44),
    S( -23,   9), S( -17,  30), S( -19,  22), S( -33,  56),
    S( -40,   2), S( -20,  18), S( -16,  21), S( -16,  43),
    S(  -5,  13), S( -54,  57), S(  11,  13), S( -44,  57),
    S( -41,  -7), S(  -5,  -4), S(  12,   6), S( -43,   6),
};

const int KingPSQT32[32] = {
    S(  79,-108), S(  88, -79), S(  41, -34), S(  21, -41),
    S(  70, -54), S(  61, -44), S(   8,  -3), S( -21,   4),
    S(  -1, -40), S(  48, -32), S(  17,   0), S( -18,  17),
    S( -53, -28), S(  38, -19), S(  -1,  14), S( -52,  35),
    S( -27, -16), S(  55,   0), S(   7,  29), S( -34,  35),
    S(  31, -13), S(  91,  -1), S(  76,  20), S(  29,  22),
    S(  -9, -14), S(  40,  -2), S(  32,  -2), S(  -9,  -1),
    S(  41, -87), S(  74, -76), S( -22, -35), S( -33, -40),
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
