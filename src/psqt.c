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
    S( -23,   2), S(  13,   3), S(  -6,   6), S(  -5,   0),
    S( -26,   0), S(  -4,  -1), S(  -6,  -6), S(  -2, -11),
    S( -21,   7), S(  -7,   6), S(   5, -10), S(   3, -23),
    S( -10,  15), S(   3,   9), S(   0,  -2), S(   4, -23),
    S(   1,  29), S(  13,  28), S(  18,   6), S(  24, -21),
    S( -46,   7), S( -34,  11), S(  -4, -16), S(   1, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -40, -48), S(  -7, -42), S( -15, -17), S(  -1, -14),
    S(   0, -47), S(   8, -12), S(  -1, -22), S(  11,  -9),
    S(  -4, -22), S(  24, -16), S(  10,  -2), S(  18,  18),
    S(  17,   4), S(  25,   9), S(  31,  29), S(  29,  37),
    S(  30,   7), S(  35,  14), S(  41,  40), S(  30,  48),
    S( -26,  10), S(  23,   6), S(  37,  39), S(  39,  35),
    S( -27, -17), S( -33,   9), S(  44, -26), S(  20,   5),
    S(-168, -34), S(-102, -29), S(-156,  -4), S( -40, -23),
};

const int BishopPSQT32[32] = {
    S(  26, -24), S(  20, -29), S( -15, -12), S(   7, -22),
    S(  34, -31), S(  24, -31), S(  19, -16), S(   2,  -8),
    S(  25, -14), S(  24, -15), S(  12,  -2), S(  12,   0),
    S(  15, -10), S(  17,   0), S(   9,  10), S(  19,  13),
    S( -19,   5), S(  32,  -1), S(   1,  13), S(  13,  17),
    S(  -9,   0), S(  -4,   6), S(  20,   4), S(  16,   5),
    S( -63,   5), S( -11,  -3), S(  -6,  -7), S( -40,   1),
    S( -50,   0), S( -63,  -2), S(-127,   6), S(-113,  12),
};

const int RookPSQT32[32] = {
    S(   0, -38), S(  -9, -22), S(   4, -26), S(  11, -35),
    S( -51, -28), S( -12, -34), S(  -7, -32), S(  -1, -38),
    S( -20, -23), S(   0, -19), S( -13, -25), S(  -5, -25),
    S( -14,  -4), S(  -8,   7), S(  -5,   1), S(  -1,   1),
    S(  -4,  16), S(   1,  19), S(  20,   8), S(  25,   8),
    S( -11,  24), S(  19,  16), S(  10,  18), S(  20,  14),
    S(  -1,  18), S( -10,  21), S(  29,   1), S(  20,  12),
    S(   4,  25), S(  14,  18), S( -17,  30), S(   4,  28),
};

const int QueenPSQT32[32] = {
    S(   3, -46), S( -13, -30), S(  -7, -24), S(  10, -43),
    S(  11, -46), S(  17, -38), S(  21, -60), S(   8, -18),
    S(   7, -22), S(  24, -20), S(   3,   4), S(  -4,  -1),
    S(   8,  -5), S(  11,   6), S(  -5,  18), S( -17,  46),
    S(   0,  11), S(  -9,  35), S( -13,  22), S( -30,  53),
    S( -13,   5), S(  -7,  19), S(  -7,  19), S( -12,  45),
    S(   0,  14), S( -68,  58), S(  16,   9), S( -24,  65),
    S( -15, -18), S(   4, -10), S(   9,  -3), S( -15,  12),
};

const int KingPSQT32[32] = {
    S(  86,-106), S(  92, -77), S(  40, -38), S(  14, -54),
    S(  64, -56), S(  55, -49), S(   0, -12), S( -18, -10),
    S(   0, -39), S(  43, -33), S(  16,  -4), S( -15,   9),
    S( -51, -29), S(  34, -13), S(   3,  21), S( -46,  34),
    S( -18, -15), S(  56,   4), S(   9,  36), S( -31,  42),
    S(  40, -16), S(  85,   3), S(  75,  26), S(  10,  23),
    S(  17, -16), S(  52,  -1), S(  35,   2), S(   9,   2),
    S(  29, -81), S(  86, -66), S( -21, -34), S( -15, -34),
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
