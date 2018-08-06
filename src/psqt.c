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
    S( -35,   6), S(  20,   4), S(  -2,   9), S( -16,  14),
    S( -36,   0), S(   0,   0), S(  -1,  -9), S(  -4,  -9),
    S( -33,  12), S( -11,  11), S(   5, -15), S(   4, -31),
    S( -17,  20), S(   6,  14), S(   9,  -2), S(  10, -32),
    S(   0,  23), S(  20,  29), S(  38,   2), S(  42, -26),
    S( -44,  12), S( -27,  14), S(  -1, -12), S(   4, -31),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -40, -49), S( -10, -47), S( -18, -19), S(   3,  -7),
    S(   0, -51), S(  10,  -8), S(  18, -23), S(  24,  -1),
    S(   1, -25), S(  38, -19), S(  29,   1), S(  44,  12),
    S(   0,  -3), S(  41,  13), S(  57,  29), S(  62,  35),
    S(  28,   0), S(  52,  17), S(  66,  45), S(  56,  53),
    S( -34,   4), S(  24,   3), S(  37,  37), S(  50,  36),
    S( -37, -26), S( -33,   6), S(  31, -32), S(   8,  -4),
    S(-162, -33), S(-102, -30), S(-158,  -8), S( -38, -27),
};
const int BishopPSQT32[32] = {
    S(  26, -23), S(  20, -29), S(   5, -12), S(  19, -12),
    S(  46, -29), S(  55, -35), S(  40, -16), S(  25,  -2),
    S(  34, -11), S(  48, -15), S(  43,   3), S(  35,  12),
    S(  20,  -3), S(  25,  -2), S(  32,  13), S(  46,  24),
    S( -14,  10), S(  39,   4), S(   0,  18), S(  37,  24),
    S( -28,   1), S( -13,   2), S(  -3,   8), S(  12,  12),
    S( -92,  -2), S(   1,  -1), S( -12, -12), S( -44,   2),
    S( -45,   1), S( -68,  -3), S(-123,   8), S(-106,  15),
};
const int RookPSQT32[32] = {
    S(  -2, -37), S(  -3, -22), S(  23, -28), S(  34, -30),
    S( -46, -28), S(  -6, -31), S(   6, -23), S(  20, -31),
    S( -22, -14), S(  10,  -8), S(  -2, -13), S(   8, -21),
    S( -13,   2), S(  -2,  13), S(   3,   8), S(   4,   7),
    S(  -4,  15), S(  -2,  18), S(  26,  13), S(  29,  14),
    S( -14,  13), S(  19,  11), S(   6,  17), S(  20,  19),
    S(   5,  23), S(   4,  23), S(  40,   6), S(  18,  11),
    S(   1,  25), S(  12,  17), S( -28,  20), S(   0,  29),
};
const int QueenPSQT32[32] = {
    S( -15, -50), S( -45, -35), S( -40, -28), S( -19, -43),
    S(  -3, -51), S(  -1, -41), S(   0, -59), S( -12,  -8),
    S(   0, -22), S(  12, -14), S(   7,  13), S(   2,   5),
    S(  -9,   0), S(  50,  13), S(  25,  23), S(  49,  49),
    S(  -6,  16), S(  14,  42), S(  28,  32), S(  23,  62),
    S(  -7,   7), S(  -1,  22), S(   8,  26), S(   1,  52),
    S(   3,  15), S( -49,  57), S(  31,  16), S( -15,  70),
    S( -22, -21), S(   2, -11), S(   5,  -4), S( -23,   8),
};
const int KingPSQT32[32] = {
    S(  91,-123), S( 100, -95), S(  20, -34), S(  14, -53),
    S(  82, -55), S(  61, -40), S(   9,   3), S( -35,  17),
    S(  -2, -48), S(  47, -34), S(  18,   2), S( -12,  24),
    S( -54, -37), S(  32, -21), S(   0,  13), S( -46,  43),
    S( -19, -17), S(  56,   4), S(   7,  33), S( -30,  44),
    S(  40, -15), S(  86,   3), S(  75,  24), S(   9,  19),
    S(  17, -16), S(  51,  -5), S(  35,   0), S(   8,   0),
    S(  28, -81), S(  85, -67), S( -22, -35), S( -16, -36),
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
