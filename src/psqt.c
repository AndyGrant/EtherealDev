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
    S( -25,   3), S(  11,   5), S(  -7,   5), S(  -8,  -8),
    S( -24,   0), S(   0,   0), S(  -7,  -5), S(  -7, -13),
    S( -18,   7), S(   1,   7), S(  10,  -8), S(   8, -21),
    S(  -5,  12), S(  15,  10), S(   8,  -3), S(  13, -18),
    S(  13,  13), S(  28,  16), S(  46,   7), S(  43, -16),
    S( -25, -50), S( -26, -17), S(  28, -20), S(  37, -31),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -60, -80), S(   2, -47), S( -25, -39), S(   0, -27),
    S(  -1, -52), S(   3, -21), S(   3, -37), S(  21, -15),
    S(   0, -35), S(  22, -23), S(  10,  -5), S(  30,   7),
    S(  16,   5), S(  27,   9), S(  24,  27), S(  33,  31),
    S(  34,  14), S(  36,  17), S(  42,  40), S(  45,  40),
    S(  -1,  28), S(  43,  16), S(  51,  43), S(  69,  41),
    S(  -1,  -2), S(  -7,  15), S(  81, -11), S(  58,   6),
    S(-129, -11), S( -52,   2), S(-123,  20), S(   7,  11),
};

const int BishopPSQT32[32] = {
    S(  29, -38), S(  19, -34), S(  -8, -17), S(  10, -28),
    S(  38, -39), S(  33, -29), S(  29, -18), S(   8,  -8),
    S(  22, -20), S(  40, -15), S(  21,  -1), S(  21,   2),
    S(  32,   0), S(  15,   2), S(  13,  14), S(  31,  13),
    S( -17,   7), S(  22,   7), S(   9,  15), S(  35,  19),
    S(   4,   5), S(  11,  10), S(  38,  17), S(  28,  14),
    S( -32,   4), S(   0,   0), S(   5,  -1), S( -13,  11),
    S( -48,  -8), S( -58,  -4), S(-112,   7), S(-107,  10),
};

const int RookPSQT32[32] = {
    S(  -4, -33), S(  -9, -22), S(   3, -21), S(   9, -25),
    S( -42, -31), S( -17, -32), S(  -7, -32), S(   0, -35),
    S( -27, -21), S(  -4, -18), S(  -2, -20), S(  -6, -21),
    S( -11,   3), S(   0,  10), S(   0,   5), S(  -2,   1),
    S(   2,  18), S(   3,  17), S(  36,  13), S(  35,  14),
    S(  -1,  19), S(  30,  10), S(  30,  17), S(  36,  19),
    S(   8,  18), S( -13,  14), S(  37,   0), S(  32,  13),
    S(  20,  30), S(  31,  23), S(  -7,  25), S(  21,  36),
};

const int QueenPSQT32[32] = {
    S(   2, -50), S( -14, -42), S( -15, -35), S(  13, -49),
    S(   0, -57), S(  10, -48), S(  23, -56), S(  13, -22),
    S(   7, -27), S(  27, -18), S(   8,   4), S(   4,   1),
    S(   8,  -5), S(   9,   4), S(  -1,  18), S( -11,  45),
    S(  -9,  18), S(  -8,  40), S(  -6,  32), S( -26,  55),
    S( -18,  14), S(  -5,  27), S(   1,  26), S(  -3,  48),
    S(   3,  29), S( -58,  65), S(  22,  16), S( -26,  61),
    S(  14,  -7), S(  17,  -6), S(  19,  -2), S(  -8,  12),
};

const int KingPSQT32[32] = {
    S(  72,-102), S(  93, -75), S(  39, -37), S(  16, -41),
    S(  67, -49), S(  53, -42), S(   9,  -4), S( -20,   4),
    S(  -2, -29), S(  45, -25), S(  22,   3), S( -11,  19),
    S( -34, -31), S(  51, -12), S(  12,  16), S( -60,  29),
    S( -15, -32), S(  81,   0), S(  11,  22), S( -71,  10),
    S(  29, -33), S( 109, -12), S(  91,   7), S(  -6, -10),
    S( -15, -46), S(  81, -15), S(  99,   6), S(  74,   2),
    S( -63,-147), S( 120, -62), S(  33, -18), S(  41, -34),
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
