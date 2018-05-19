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
    S( -21,   5), S(   9,   2), S(  -8,   7), S(  -6,  -6),
    S( -22,   2), S(  -4,  -1), S(  -9,  -7), S(  -5, -11),
    S( -20,   4), S(  -4,   5), S(   9,  -9), S(   5, -20),
    S(  -8,   7), S(   8,   4), S(   3,  -6), S(   7, -20),
    S(   6,  -8), S(  30,  11), S(  28,  -4), S(  32, -21),
    S( -27, -79), S( -33, -22), S(  12, -31), S(   9, -49),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S(  -2, -41), S(   5, -41), S( -10, -32), S(   6, -24),
    S(   3, -62), S(  13, -18), S(  10, -32), S(  15, -16),
    S(   4, -27), S(  23, -23), S(   7,  -7), S(  21,   6),
    S(  16,   7), S(  25,   4), S(  24,  25), S(  28,  23),
    S(  32,  17), S(  28,   6), S(  35,  29), S(  38,  28),
    S(  -2,  21), S(  44,  11), S(  43,  40), S(  50,  27),
    S(  -2,  -6), S( -16,   7), S(  63, -24), S(  50,  -5),
    S(-157, -13), S( -75, -12), S(-106,   1), S( -14, -17),
};

const int BishopPSQT32[32] = {
    S(  32, -40), S(  30, -47), S(  -4, -17), S(   7, -23),
    S(  38, -35), S(  29, -32), S(  30, -31), S(   8, -12),
    S(  25, -19), S(  30, -16), S(  21,  -8), S(  18,  -7),
    S(  25,  -7), S(   9,  -6), S(   8,   8), S(  28,   0),
    S(  -6,   9), S(  17,   2), S(  17,   1), S(  23,  15),
    S(  -6,   8), S(  23,  -1), S(  33,   6), S(  18,  14),
    S( -41,   0), S(   0, -14), S(   2,  -3), S( -21,   4),
    S( -38, -21), S( -43,   0), S(-113,   1), S( -81,   5),
};

const int RookPSQT32[32] = {
    S(  -3, -34), S(  -5, -26), S(   0, -23), S(   6, -27),
    S( -34, -30), S( -11, -36), S(  -5, -34), S(   1, -32),
    S( -20, -15), S(  -2, -19), S(  -8, -21), S(  -8, -24),
    S( -13,   1), S(  -6,   0), S(  -6,   1), S(  -5,  -2),
    S(  -2,   7), S(   0,   3), S(  22,   2), S(  18,   0),
    S(   1,   8), S(  22,  -1), S(  15,   1), S(  31,   4),
    S(   0,   5), S(  -8,   5), S(  29,  -7), S(  26,  -3),
    S(  12,  10), S(  24,   9), S(  -9,  19), S(  12,  16),
};

const int QueenPSQT32[32] = {
    S(   9, -60), S(  -4, -40), S(  -3, -38), S(  12, -45),
    S(  11, -61), S(  15, -49), S(  23, -59), S(  13, -19),
    S(   4, -29), S(  20, -21), S(   9,   1), S(   5,  -1),
    S(   7,  -5), S(   7,   0), S(  -5,  10), S( -10,  36),
    S( -16,   9), S( -17,  27), S( -12,  12), S( -28,  45),
    S( -22,   0), S(  -5,  14), S( -13,  15), S( -17,  31),
    S(   3,  10), S( -47,  54), S(   9,   3), S( -35,  39),
    S(  -1, -21), S(   3, -32), S(  11, -18), S( -26, -11),
};

const int KingPSQT32[32] = {
    S(  67, -57), S(  88, -63), S(  42, -27), S(  21, -31),
    S(  65, -38), S(  55, -35), S(  13,  -3), S( -15,   1),
    S(   0, -40), S(  41, -30), S(  21,   0), S( -12,  17),
    S( -39, -35), S(  39, -26), S(   0,   4), S( -58,  24),
    S(   3, -54), S(  61, -53), S( -11,   0), S( -61,   6),
    S(  40, -39), S( 105, -50), S(  65,  -8), S(   8, -11),
    S(  10, -56), S(  64, -23), S(  48,  -1), S(  47,  -1),
    S( -52,-135), S( 153, -20), S(  42,  -6), S(  59, -16),
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
