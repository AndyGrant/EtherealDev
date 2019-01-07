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
    S( -14,   4), S(  17,  -3), S(   4,   0), S(   5,  -4),
    S( -21,   6), S(   0,   0), S(  -1,  -5), S(   8, -13),
    S( -14,  15), S(  -1,   4), S(  13,  -9), S(  12, -21),
    S(  -3,  16), S(   8,   5), S(   3,  -2), S(  10, -20),
    S(   1,  24), S(   8,  22), S(  14,   2), S(  28, -21),
    S( -46,   5), S( -37,   5), S(   0, -18), S(   3, -29),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -37, -47), S(   0, -28), S(  -7, -12), S(   3,  -5),
    S(   6, -44), S(   7, -11), S(   6, -13), S(  12,  -1),
    S(   7, -27), S(  23, -18), S(  15,   1), S(  27,  14),
    S(   2,   7), S(  25,   8), S(  33,  33), S(  54,  33),
    S(  29,   7), S(  44,  10), S(  45,  42), S(  54,  48),
    S( -28,  10), S(  18,   8), S(  43,  37), S(  44,  34),
    S( -32, -17), S( -38,   5), S(  31, -36), S(  14,   0),
    S(-170, -35), S(-102, -30), S(-155,  -6), S( -39, -28),
};

const int BishopPSQT32[32] = {
    S(  28, -20), S(  17, -26), S(  -1,  -6), S(  21, -13),
    S(  32, -30), S(  27, -24), S(  21,  -7), S(  12,   0),
    S(  22,  -8), S(  29, -10), S(  21,   0), S(  22,   6),
    S(   4,  -2), S(  25,  -2), S(  20,  11), S(  34,  18),
    S( -18,  16), S(  39,   3), S(   3,  17), S(  30,  18),
    S(   1,   6), S(   0,  12), S(  27,  10), S(  23,  10),
    S( -69,   4), S(  -7,  -3), S(  -8,  -4), S( -39,   1),
    S( -47,  -1), S( -60,   0), S(-123,   4), S(-111,  11),
};

const int RookPSQT32[32] = {
    S(   1, -28), S(  -5, -15), S(   8, -19), S(  14, -31),
    S( -41, -22), S(  -2, -29), S(   0, -24), S(  11, -36),
    S( -11, -18), S(  10, -14), S(  -4, -21), S(   7, -23),
    S( -10,  -3), S(  -7,   6), S(  -2,   3), S(   6,   4),
    S(  -7,  14), S(   1,  12), S(  16,   5), S(  30,   7),
    S( -14,  15), S(  17,   9), S(   8,  16), S(  21,  11),
    S(  -2,  14), S(  -3,  18), S(  29,   1), S(  21,  10),
    S(   1,  26), S(  13,  15), S( -19,  28), S(   0,  30),
};

const int QueenPSQT32[32] = {
    S(   2, -47), S( -18, -28), S(   0, -22), S(  12, -36),
    S(  10, -44), S(  16, -37), S(  18, -61), S(   9, -10),
    S(   5, -20), S(  24, -17), S(   6,   9), S(   0,   0),
    S(   4,  -5), S(  14,   3), S(  -6,  17), S(   0,  46),
    S(  -7,  10), S( -13,  34), S(  -6,  23), S( -16,  51),
    S( -12,   4), S( -11,  17), S(  -2,  21), S(  -7,  42),
    S(  -1,  11), S( -75,  57), S(  19,   9), S( -20,  62),
    S( -14, -21), S(   3, -10), S(   6,  -5), S( -17,   8),
};

const int KingPSQT32[32] = {
    S(  82,-105), S(  90, -80), S(  34, -30), S(  25, -35),
    S(  66, -48), S(  50, -35), S(   1,   3), S( -14,  10),
    S(   3, -40), S(  43, -24), S(  17,   5), S(  -9,  24),
    S( -52, -38), S(  34, -19), S(   3,  22), S( -44,  41),
    S( -19, -22), S(  54,  -3), S(   9,  31), S( -29,  39),
    S(  36, -26), S(  82,  -5), S(  74,  14), S(   8,  12),
    S(  14, -19), S(  49,  -9), S(  31,  -6), S(   6,  -4),
    S(  26, -81), S(  83, -68), S( -22, -36), S( -16, -36),
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
