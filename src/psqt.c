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
    S( -22,   1), S(  15,   3), S(  -4,   6), S(  -5,   1),
    S( -24,   0), S(  -1,   0), S(  -6,  -6), S(  -2, -10),
    S( -21,   7), S(  -6,   6), S(   5, -12), S(   4, -23),
    S( -10,  15), S(   0,  10), S(  -1,  -2), S(   3, -24),
    S(  -2,  33), S(  19,  33), S(  20,  10), S(  27, -15),
    S( -48,  18), S( -43,  18), S(   0,  -9), S(   6, -27),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -33, -43), S(   4, -35), S(  -5, -15), S(   9, -10),
    S(   6, -43), S(  16, -11), S(  16, -25), S(  21,  -4),
    S(   8, -21), S(  26, -17), S(  16,   1), S(  29,  10),
    S(   8,   2), S(  17,   7), S(  27,  25), S(  32,  31),
    S(  24,   7), S(  25,  13), S(  34,  35), S(  36,  39),
    S( -28,  14), S(  24,  10), S(  36,  35), S(  48,  31),
    S( -16, -23), S( -25,   7), S(  67, -30), S(  27,  -6),
    S(-161, -30), S(-104, -24), S(-134,  -3), S( -46, -26),
};
const int BishopPSQT32[32] = {
    S(  28, -24), S(  22, -30), S(   1,  -6), S(  18, -14),
    S(  39, -30), S(  35, -23), S(  25, -16), S(  12,  -4),
    S(  26, -15), S(  30, -14), S(  22,   1), S(  16,   5),
    S(  15,  -9), S(  10,  -4), S(   9,  12), S(  28,  15),
    S( -15,   8), S(  18,   1), S(   0,  14), S(  21,  20),
    S(   1,   4), S(  -7,   8), S(  26,   6), S(  25,   4),
    S( -67,   5), S(  13,  -5), S(   0, -13), S( -28,  -3),
    S( -35,   1), S( -54,  -4), S(-109,   5), S(-111,   9),
};
const int RookPSQT32[32] = {
    S(  -2, -28), S(  -4, -18), S(   7, -13), S(  14, -18),
    S( -34, -21), S(  -6, -25), S(   2, -17), S(   8, -22),
    S( -19, -14), S(   1, -15), S(  -6, -14), S(   2, -19),
    S( -18,   0), S( -20,   1), S(  -8,   0), S(  -3,   0),
    S(  -9,   9), S( -15,   7), S(   9,   0), S(  12,   2),
    S( -20,  13), S(   2,   6), S(  -3,  10), S(  11,   7),
    S(  -5,  12), S( -15,  14), S(  24,   0), S(  11,   4),
    S(  -3,  16), S(   0,  11), S( -22,  20), S(  -8,  20),
};
const int QueenPSQT32[32] = {
    S(   0, -46), S(  -9, -33), S(  -2, -23), S(  17, -39),
    S(   9, -45), S(  15, -39), S(  20, -50), S(  15, -13),
    S(   7, -23), S(  22, -18), S(   7,   8), S(   4,   4),
    S(   8,  -5), S(   8,   6), S(  -6,  17), S(  -8,  41),
    S( -14,  13), S( -15,  30), S( -13,  21), S( -27,  52),
    S( -24,   5), S(  -9,  16), S(  -7,  19), S(  -8,  40),
    S(  -8,  12), S( -54,  54), S(  13,  11), S( -41,  55),
    S( -29,  -2), S(   5,  -1), S(  22,  11), S( -22,  11),
};
const int KingPSQT32[32] = {
    S(  85,-101), S(  82, -76), S(  38, -32), S(  19, -37),
    S(  72, -52), S(  53, -43), S(  11,  -4), S( -16,   3),
    S(   9, -38), S(  45, -31), S(  20,   0), S( -12,  16),
    S( -37, -26), S(  38, -17), S(   2,  14), S( -46,  34),
    S( -17, -15), S(  55,   0), S(   6,  28), S( -32,  34),
    S(  35, -12), S(  88,   0), S(  68,  19), S(  23,  21),
    S(  10, -11), S(  53,   1), S(  42,   0), S(   1,   1),
    S(  67, -78), S( 108, -66), S(   5, -27), S(  -6, -33),
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
