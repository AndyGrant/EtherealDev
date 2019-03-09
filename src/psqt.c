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
    S( -21,   9), S(   4,   3), S( -15,   9), S( -12,  -4),
    S( -20,   2), S( -15,   1), S(  -9,  -7), S(  -6, -17),
    S( -13,  13), S(  -7,  12), S(  18, -15), S(  16, -30),
    S(   2,  19), S(   8,  14), S(   3,   0), S(  21, -21),
    S(   2,  35), S(   4,  37), S(  16,  25), S(  45,   0),
    S( -18, -40), S( -65,  -7), S(   2, -18), S(  37, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -49, -26), S( -13, -48), S( -21, -33), S(  -3, -22),
    S( -10, -21), S(   1, -14), S(  -9, -33), S(   2, -20),
    S(  -3, -26), S(  15, -24), S(   5, -16), S(  22,   0),
    S(  16,   3), S(  24,   6), S(  30,  17), S(  31,  24),
    S(  28,  17), S(  30,  13), S(  44,  28), S(  35,  43),
    S( -12,  15), S(   7,  13), S(  31,  31), S(  35,  33),
    S(   5,  -9), S(  -4,   1), S(  39, -17), S(  41,   2),
    S(-163, -16), S( -82,  -2), S(-113,  17), S( -32,   0),
};

const int BishopPSQT32[32] = {
    S(  16, -17), S(  15, -20), S( -19, -11), S(   5, -14),
    S(  33, -32), S(  18, -36), S(  23, -22), S(   1,  -9),
    S(  11, -13), S(  31, -14), S(  14,  -5), S(  19,   0),
    S(  17,  -8), S(  16,   0), S(  17,   6), S(  18,  13),
    S( -11,   7), S(  21,   5), S(   5,  14), S(  10,  24),
    S(   2,   6), S(   0,  13), S(  17,  13), S(  20,   9),
    S( -45,  11), S( -32,  10), S(  -1,   4), S( -19,   7),
    S( -42,   0), S( -49,   6), S( -91,  13), S( -95,  22),
};

const int RookPSQT32[32] = {
    S( -15, -33), S( -18, -21), S(  -6, -25), S(   7, -33),
    S( -65, -18), S( -17, -33), S( -12, -36), S(  -3, -39),
    S( -32, -17), S(  -6, -12), S( -17, -16), S(  -3, -26),
    S( -17,   0), S(  -3,   6), S(  -5,   3), S(   6,  -2),
    S(   0,  12), S(  16,   9), S(  26,   5), S(  39,   2),
    S(  -6,  24), S(  30,  11), S(   2,  20), S(  35,   8),
    S(   3,  10), S( -16,  14), S(   4,   7), S(  19,   8),
    S(  40,  27), S(  28,  29), S(   5,  34), S(  17,  31),
};

const int QueenPSQT32[32] = {
    S(   6, -53), S( -10, -38), S(  -4, -53), S(   3, -48),
    S(   8, -39), S(  21, -58), S(  24, -78), S(  11, -29),
    S(   9, -20), S(  26, -17), S(   6,   4), S(   7,   0),
    S(  12,   0), S(  17,  17), S(   7,  19), S( -18,  66),
    S(  -5,  20), S(  -1,  41), S( -13,  20), S( -33,  71),
    S( -22,  27), S( -14,  18), S( -22,  17), S( -12,  22),
    S(  -6,  25), S( -68,  61), S( -11,  13), S( -41,  53),
    S( -15,   8), S(  11,  -1), S(   5,   1), S(  -9,  14),
};

const int KingPSQT32[32] = {
    S(  43, -93), S(  45, -56), S( -11, -15), S( -33, -32),
    S(  34, -31), S(  -4, -19), S( -40,   5), S( -49,   4),
    S(  11, -36), S(  25, -31), S(  24,  -5), S(   1,  10),
    S(   0, -36), S(  83, -38), S(  41,   1), S(  -3,  21),
    S(   0, -17), S(  90, -24), S(  40,  13), S(  -1,  21),
    S(  46, -17), S( 113, -12), S(  90,  12), S(  33,   9),
    S(   5, -40), S(  46,  -2), S(  32,  12), S(   7,   5),
    S(  11, -93), S(  76, -49), S( -18,  -7), S( -16,  -6),
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
