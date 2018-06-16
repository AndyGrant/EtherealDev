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
    S( -19,   3), S(  19,   2), S(  -2,   8), S(   0,   4),
    S( -23,   0), S(  -4,  -2), S(  -3, -11), S(   2, -13),
    S( -18,   8), S(  -5,   5), S(  14, -18), S(  12, -30),
    S(  -5,  18), S(   5,  13), S(  -1,  -3), S(  11, -27),
    S(   2,  37), S(  20,  35), S(  22,  12), S(  32, -14),
    S( -44,  12), S( -35,  14), S(  -2, -13), S(   2, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -39, -48), S(   6, -38), S(  -6, -15), S(  10, -12),
    S(   9, -48), S(  15, -10), S(  15, -27), S(  21, -10),
    S(  11, -24), S(  26, -20), S(  14,  -6), S(  28,   7),
    S(  10,   0), S(  19,   6), S(  26,  19), S(  29,  30),
    S(  26,   6), S(  23,  13), S(  31,  35), S(  34,  42),
    S( -29,  10), S(  23,   7), S(  35,  33), S(  46,  30),
    S( -31, -22), S( -34,   7), S(  55, -29), S(  16,  -4),
    S(-171, -37), S(-103, -30), S(-158,  -7), S( -42, -28),
};
const int BishopPSQT32[32] = {
    S(  27, -24), S(  22, -29), S(   0,  -7), S(  19, -15),
    S(  39, -31), S(  36, -29), S(  26, -18), S(  12,  -8),
    S(  27, -18), S(  33, -16), S(  23,   0), S(  17,   4),
    S(  13,  -9), S(   9,  -5), S(  10,   9), S(  28,  14),
    S( -17,   8), S(  18,   1), S(   0,  13), S(  20,  21),
    S(  -1,   5), S(  -4,   8), S(  24,   6), S(  22,   5),
    S( -69,   4), S(   2,  -4), S(  -7, -13), S( -41,  -1),
    S( -50,   0), S( -64,  -3), S(-127,   6), S(-114,  11),
};
const int RookPSQT32[32] = {
    S(   0, -27), S(   0, -20), S(  11, -14), S(  19, -18),
    S( -34, -21), S(  -4, -25), S(   6, -17), S(  12, -20),
    S( -18, -14), S(   4, -15), S(  -3, -13), S(   6, -18),
    S( -18,   0), S( -15,   1), S(  -5,   0), S(   0,   0),
    S(  -9,  10), S( -13,   9), S(  14,   0), S(  17,   3),
    S( -19,  13), S(  10,   5), S(   4,  10), S(  16,   8),
    S(  -3,  10), S( -13,  13), S(  31,  -2), S(  16,   3),
    S(   0,  18), S(   9,  12), S( -23,  22), S(   0,  21),
};
const int QueenPSQT32[32] = {
    S(   0, -47), S(  -8, -33), S(   0, -25), S(  20, -46),
    S(   8, -48), S(  16, -41), S(  23, -54), S(  18, -16),
    S(   7, -24), S(  25, -20), S(   9,   8), S(   7,   3),
    S(   9,  -6), S(   8,   5), S(  -5,  17), S(  -8,  43),
    S( -14,  11), S( -16,  31), S( -13,  22), S( -30,  54),
    S( -22,   2), S(  -8,  17), S(  -7,  19), S( -11,  44),
    S(  -9,  11), S( -68,  56), S(  18,   9), S( -29,  60),
    S( -25, -18), S(   2, -10), S(  10,   0), S( -21,   9),
};
const int KingPSQT32[32] = {
    S(  86,-109), S(  86, -74), S(  39, -33), S(  22, -41),
    S(  74, -56), S(  53, -43), S(   5,  -3), S( -21,   5),
    S(   2, -40), S(  46, -33), S(  16,   0), S( -15,  19),
    S( -51, -29), S(  34, -19), S(   0,  15), S( -47,  36),
    S( -19, -18), S(  55,  -1), S(   7,  28), S( -32,  35),
    S(  39, -16), S(  84,  -1), S(  73,  19), S(  10,  21),
    S(  16, -16), S(  51,  -3), S(  35,   0), S(   8,   0),
    S(  29, -81), S(  86, -67), S( -21, -34), S( -15, -35),
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
