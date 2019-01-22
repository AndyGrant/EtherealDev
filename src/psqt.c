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
    S( -20,  10), S(   5,   3), S( -15,   6), S(  -9,  -3),
    S( -21,   2), S( -13,   2), S(  -9,  -7), S(  -5, -16),
    S( -16,  12), S( -10,  12), S(  14, -14), S(  13, -28),
    S(  -2,  18), S(   5,  13), S(   3,  -1), S(  17, -21),
    S(   0,  33), S(   3,  36), S(  15,  24), S(  43,   0),
    S( -19, -40), S( -65,  -7), S(   2, -19), S(  37, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -48, -27), S(  -7, -48), S( -17, -31), S(  -1, -21),
    S(  -7, -22), S(   4, -14), S(   0, -32), S(  10, -20),
    S(   2, -26), S(  21, -25), S(  13, -17), S(  24,  -1),
    S(  19,   4), S(  24,   7), S(  31,  18), S(  30,  23),
    S(  28,  18), S(  28,  11), S(  41,  28), S(  31,  40),
    S( -13,  16), S(   9,  13), S(  32,  32), S(  35,  32),
    S(   6, -10), S(  -5,   3), S(  41, -19), S(  40,   1),
    S(-162, -17), S( -84,  -4), S(-116,  18), S( -33,   0),
};

const int BishopPSQT32[32] = {
    S(  18, -18), S(  16, -21), S( -10, -10), S(   9, -14),
    S(  34, -31), S(  27, -33), S(  25, -21), S(  10, -11),
    S(  18, -13), S(  32, -14), S(  17,  -4), S(  20,  -2),
    S(  18,  -8), S(  18,  -1), S(  17,   7), S(  18,  13),
    S(  -9,   8), S(  19,   3), S(   6,  15), S(  11,  24),
    S(   2,   5), S(   0,  13), S(  17,  12), S(  22,  10),
    S( -45,  10), S( -31,   7), S(  -1,   1), S( -21,   3),
    S( -43,   0), S( -51,   4), S( -94,  12), S( -98,  19),
};

const int RookPSQT32[32] = {
    S(  -6, -30), S( -13, -21), S(   0, -25), S(   7, -29),
    S( -54, -15), S( -16, -31), S( -10, -31), S(  -2, -34),
    S( -28, -14), S(  -7, -12), S( -17, -16), S(  -4, -24),
    S( -14,   0), S(  -5,   6), S(  -7,   2), S(   3,  -3),
    S(   0,  11), S(  13,   9), S(  24,   4), S(  34,   0),
    S(  -7,  22), S(  28,   9), S(   0,  19), S(  31,   5),
    S(   4,   8), S( -13,  15), S(   5,   7), S(  18,   5),
    S(  36,  21), S(  26,  23), S(   3,  28), S(  13,  24),
};

const int QueenPSQT32[32] = {
    S(   7, -53), S(  -9, -38), S(   0, -52), S(  13, -46),
    S(   9, -39), S(  22, -57), S(  25, -73), S(  15, -26),
    S(   7, -21), S(  23, -18), S(   5,   3), S(   4,   2),
    S(   8,  -1), S(  12,  15), S(  -1,  17), S( -17,  63),
    S(  -9,  21), S(  -6,  40), S( -15,  19), S( -33,  70),
    S( -24,  27), S( -15,  20), S( -20,  18), S( -12,  22),
    S(  -7,  26), S( -63,  61), S(  -9,  15), S( -42,  54),
    S( -19,   6), S(  11,  -2), S(   5,   0), S( -12,  13),
};

const int KingPSQT32[32] = {
    S(  41, -80), S(  42, -51), S(  -9, -12), S( -27, -20),
    S(  33, -34), S(   1, -25), S( -33,   2), S( -47,   5),
    S(  14, -37), S(  26, -33), S(  22,  -8), S(  -2,   7),
    S(   0, -37), S(  83, -40), S(  41,   0), S(  -5,  18),
    S(   1, -21), S(  89, -26), S(  39,  11), S(  -3,  19),
    S(  46, -19), S( 112, -13), S(  89,  10), S(  32,   9),
    S(   7, -41), S(  47,  -4), S(  33,  11), S(   8,   6),
    S(  13, -92), S(  78, -50), S( -18,  -8), S( -16,  -7),
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
