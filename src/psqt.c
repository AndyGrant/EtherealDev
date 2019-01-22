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
    S( -20,   9), S(   5,   4), S( -15,   7), S(  -9,  -3),
    S( -21,   2), S( -13,   3), S( -10,  -7), S(  -5, -15),
    S( -16,  12), S( -10,  12), S(  14, -14), S(  13, -27),
    S(  -2,  18), S(   5,  13), S(   3,  -1), S(  17, -21),
    S(   0,  32), S(   4,  36), S(  16,  24), S(  43,   0),
    S( -19, -41), S( -65,  -7), S(   3, -19), S(  37, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -49, -27), S(  -8, -49), S( -18, -32), S(  -2, -22),
    S(  -7, -22), S(   3, -15), S(   0, -33), S(  10, -21),
    S(   2, -26), S(  21, -25), S(  13, -17), S(  24,  -1),
    S(  19,   4), S(  24,   7), S(  31,  18), S(  30,  23),
    S(  28,  17), S(  28,  12), S(  41,  28), S(  31,  40),
    S( -13,  16), S(   9,  14), S(  31,  32), S(  35,  32),
    S(   6, -11), S(  -5,   2), S(  41, -19), S(  40,   1),
    S(-163, -17), S( -84,  -4), S(-116,  17), S( -34,  -1),
};

const int BishopPSQT32[32] = {
    S(  17, -18), S(  16, -21), S( -10, -11), S(   8, -15),
    S(  34, -32), S(  27, -33), S(  24, -22), S(  11, -11),
    S(  18, -14), S(  32, -14), S(  17,  -4), S(  20,  -2),
    S(  18,  -8), S(  17,  -1), S(  17,   8), S(  17,  13),
    S(  -9,   8), S(  19,   4), S(   5,  15), S(  11,  24),
    S(   2,   6), S(   0,  13), S(  17,  12), S(  22,  10),
    S( -45,  10), S( -32,   7), S(  -1,   1), S( -21,   3),
    S( -44,   0), S( -51,   4), S( -94,  12), S( -98,  19),
};

const int RookPSQT32[32] = {
    S(  -7, -31), S( -13, -21), S(   0, -25), S(   7, -28),
    S( -54, -15), S( -16, -31), S( -11, -32), S(  -2, -34),
    S( -27, -14), S(  -7, -12), S( -18, -16), S(  -4, -24),
    S( -15,   0), S(  -5,   6), S(  -7,   2), S(   3,  -3),
    S(   0,  12), S(  13,   9), S(  24,   4), S(  35,   0),
    S(  -8,  21), S(  28,   9), S(   0,  19), S(  31,   5),
    S(   5,   9), S( -13,  16), S(   4,   7), S(  18,   6),
    S(  37,  20), S(  26,  23), S(   3,  28), S(  14,  24),
};

const int QueenPSQT32[32] = {
    S(   7, -54), S( -10, -38), S(  -1, -53), S(  13, -47),
    S(   8, -40), S(  21, -58), S(  24, -74), S(  15, -26),
    S(   7, -21), S(  23, -19), S(   5,   3), S(   4,   2),
    S(   8,  -1), S(  12,  15), S(  -2,  16), S( -18,  62),
    S(  -9,  22), S(  -7,  39), S( -16,  18), S( -34,  69),
    S( -23,  27), S( -15,  19), S( -20,  17), S( -13,  21),
    S(  -6,  27), S( -63,  61), S( -10,  14), S( -43,  53),
    S( -20,   6), S(  10,  -3), S(   4,  -1), S( -13,  12),
};

const int KingPSQT32[32] = {
    S(  41, -80), S(  42, -50), S( -10, -12), S( -28, -20),
    S(  34, -33), S(   1, -24), S( -32,   3), S( -46,   6),
    S(  15, -36), S(  27, -33), S(  23,  -7), S(  -1,   8),
    S(   0, -38), S(  82, -41), S(  41,   0), S(  -6,  17),
    S(   1, -22), S(  89, -27), S(  39,  10), S(  -4,  18),
    S(  46, -20), S( 112, -14), S(  89,   9), S(  32,   8),
    S(   6, -42), S(  46,  -4), S(  32,  11), S(   8,   6),
    S(  13, -92), S(  78, -50), S( -19,  -8), S( -16,  -6),
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
