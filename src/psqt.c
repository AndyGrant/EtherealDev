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
    S( -22,   2), S(  15,   2), S(  -7,   6), S(  -4,   1),
    S( -27,  -1), S(  -8,  -2), S(  -7, -12), S(  -1, -16),
    S( -21,   7), S(  -8,   5), S(   9, -19), S(   8, -32),
    S(  -9,  17), S(   1,  13), S(  -3,  -4), S(   7, -29),
    S(   0,  36), S(  18,  35), S(  20,  11), S(  29, -16),
    S( -45,  11), S( -36,  13), S(  -3, -13), S(   2, -31),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -39, -48), S(   9, -36), S(  -4, -15), S(  12, -11),
    S(  10, -47), S(  16, -10), S(  17, -27), S(  23,  -9),
    S(  13, -23), S(  27, -19), S(  16,  -2), S(  29,   9),
    S(  12,   0), S(  19,   6), S(  28,  20), S(  31,  31),
    S(  26,   7), S(  25,  14), S(  33,  35), S(  35,  42),
    S( -29,  11), S(  23,   7), S(  35,  33), S(  46,  30),
    S( -30, -21), S( -33,   7), S(  56, -29), S(  17,  -3),
    S(-170, -36), S(-103, -29), S(-157,  -6), S( -42, -27),
};
const int BishopPSQT32[32] = {
    S(  28, -23), S(  23, -29), S(   1,  -6), S(  20, -14),
    S(  40, -30), S(  37, -26), S(  28, -17), S(  14,  -7),
    S(  29, -17), S(  34, -15), S(  24,   2), S(  18,   6),
    S(  14,  -8), S(  11,  -5), S(  11,  11), S(  30,  15),
    S( -15,   8), S(  19,   2), S(   0,  14), S(  22,  23),
    S(   0,   4), S(  -4,   8), S(  25,   7), S(  23,   6),
    S( -68,   4), S(   4,  -4), S(  -6, -12), S( -39,  -1),
    S( -49,   0), S( -63,  -3), S(-127,   7), S(-114,  11),
};
const int RookPSQT32[32] = {
    S(   3, -28), S(   2, -20), S(  14, -15), S(  21, -19),
    S( -31, -20), S(  -2, -25), S(   7, -17), S(  13, -21),
    S( -16, -13), S(   6, -15), S(  -1, -12), S(   7, -18),
    S( -15,   0), S( -15,   1), S(  -4,   0), S(   1,   1),
    S(  -8,  10), S( -12,   9), S(  14,   1), S(  17,   3),
    S( -18,  14), S(  10,   6), S(   4,  10), S(  16,   7),
    S(  -3,   9), S( -12,  12), S(  30,  -2), S(  17,   2),
    S(   0,  17), S(   9,  11), S( -23,  22), S(   0,  20),
};
const int QueenPSQT32[32] = {
    S(   0, -47), S(  -7, -32), S(   0, -24), S(  21, -44),
    S(   9, -48), S(  17, -40), S(  24, -53), S(  19, -14),
    S(   8, -23), S(  26, -19), S(   9,   9), S(   7,   4),
    S(  10,  -6), S(   9,   6), S(  -4,  18), S(  -7,  43),
    S( -13,  12), S( -15,  32), S( -13,  22), S( -29,  54),
    S( -22,   3), S(  -8,  17), S(  -7,  19), S( -10,  43),
    S(  -9,  11), S( -65,  57), S(  17,   9), S( -30,  60),
    S( -25, -16), S(   2,  -9), S(  11,   1), S( -21,   9),
};
const int KingPSQT32[32] = {
    S(  85,-107), S(  85, -73), S(  39, -32), S(  22, -40),
    S(  74, -56), S(  53, -43), S(   6,  -3), S( -21,   5),
    S(   2, -40), S(  46, -34), S(  16,   0), S( -15,  18),
    S( -51, -28), S(  33, -19), S(   0,  14), S( -47,  36),
    S( -19, -18), S(  55,  -2), S(   7,  28), S( -32,  35),
    S(  39, -16), S(  84,  -1), S(  73,  19), S(  10,  21),
    S(  16, -16), S(  51,  -3), S(  35,   0), S(   8,   0),
    S(  29, -80), S(  86, -67), S( -21, -34), S( -15, -35),
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
