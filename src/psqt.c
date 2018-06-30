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
    S( -11,   0), S(  27,  -2), S(   6,   1), S(   8,  -1),
    S( -15,  -4), S(   4,  -6), S(   5, -17), S(  11, -21),
    S( -10,   4), S(   0,   3), S(  23, -25), S(  20, -38),
    S(   2,  15), S(  11,  11), S(   6,  -6), S(  18, -33),
    S(  10,  38), S(  29,  37), S(  30,  13), S(  44, -17),
    S( -43,  15), S( -39,  16), S(   0, -11), S(   5, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -38, -47), S(   2, -38), S(  -8, -18), S(   8, -16),
    S(   6, -43), S(  14, -13), S(  11, -29), S(  16, -11),
    S(   8, -26), S(  24, -21), S(  13,  -4), S(  25,   6),
    S(  19,  -1), S(  27,  11), S(  37,  20), S(  37,  32),
    S(  31,  10), S(  33,  17), S(  40,  36), S(  39,  46),
    S( -28,  15), S(  22,  14), S(  38,  32), S(  53,  30),
    S( -23, -23), S( -33,   8), S(  65, -34), S(  20,  -9),
    S(-180, -35), S(-103, -27), S(-155,  -3), S( -45, -28),
};
const int BishopPSQT32[32] = {
    S(  26, -27), S(  20, -32), S(  -3,  -6), S(  15, -15),
    S(  37, -33), S(  33, -27), S(  24, -20), S(   9,  -8),
    S(  25, -22), S(  32, -17), S(  22,   0), S(  15,   4),
    S(  20, -11), S(  17,  -5), S(  16,  13), S(  36,  16),
    S(  -9,   9), S(  29,   1), S(   6,  15), S(  26,  27),
    S(   5,   3), S(  -5,  11), S(  28,   8), S(  32,   4),
    S( -72,   7), S(   4,  -6), S(  -3, -14), S( -36,  -3),
    S( -51,   2), S( -62,  -2), S(-125,   9), S(-116,  11),
};
const int RookPSQT32[32] = {
    S(   3, -28), S(   2, -21), S(  14, -16), S(  22, -21),
    S( -33, -17), S(  -1, -23), S(   9, -17), S(  13, -20),
    S( -16, -10), S(  10, -16), S(   0,  -9), S(  10, -18),
    S( -12,   1), S(  -7,   4), S(   6,  -2), S(   9,   1),
    S(  -2,   8), S(  -4,   9), S(  21,   0), S(  21,   4),
    S( -21,  13), S(   5,   6), S(  -4,  12), S(  18,   4),
    S(  -2,   2), S( -17,   8), S(  24,  -6), S(  13,  -3),
    S(   0,  14), S(   5,  11), S( -22,  20), S(  -1,  17),
};
const int QueenPSQT32[32] = {
    S(   3, -50), S(  -5, -40), S(   2, -33), S(  22, -49),
    S(  11, -49), S(  21, -48), S(  25, -57), S(  20, -17),
    S(  10, -27), S(  29, -22), S(  11,  12), S(  10,   3),
    S(  14,  -8), S(  18,   8), S(   2,  21), S(   0,  38),
    S(  -7,  13), S(  -6,  28), S(  -7,  23), S( -27,  60),
    S( -26,   5), S( -10,  15), S( -12,  20), S( -11,  39),
    S( -12,  12), S( -63,  55), S(  11,  10), S( -42,  53),
    S( -39,  -1), S(   1,  -4), S(  14,  12), S( -24,   9),
};
const int KingPSQT32[32] = {
    S(  85,-106), S(  82, -68), S(  38, -29), S(  23, -39),
    S(  72, -54), S(  49, -39), S(   3,  -1), S( -24,   7),
    S(   4, -41), S(  52, -37), S(  18,  -1), S( -15,  19),
    S( -50, -25), S(  38, -21), S(   1,  14), S( -47,  36),
    S( -20, -18), S(  55,  -8), S(   7,  25), S( -32,  33),
    S(  38, -16), S(  84,  -8), S(  72,  16), S(  12,  23),
    S(  16, -16), S(  51,  -5), S(  35,  -1), S(   7,  -1),
    S(  29, -80), S(  86, -68), S( -20, -32), S( -15, -35),
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
