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
    S( -21,   2), S(  10,   3), S(  -6,   5), S(  -6,  -3),
    S( -22,   0), S(  -1,   0), S(  -5,  -4), S(  -5, -11),
    S( -18,   6), S(  -2,   6), S(   6,  -8), S(   6, -19),
    S(  -7,  12), S(   7,   8), S(   4,  -2), S(   8, -18),
    S(   2,  19), S(  15,  19), S(  27,   6), S(  29, -17),
    S( -41, -16), S( -31,  -2), S(   6, -17), S(  13, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -47, -56), S(   3, -38), S( -17, -23), S(   2, -15),
    S(   1, -47), S(   4, -14), S(   7, -28), S(  19,  -7),
    S(   1, -24), S(  21, -19), S(  12,   0), S(  26,   9),
    S(  10,   6), S(  24,   7), S(  25,  26), S(  31,  29),
    S(  28,   8), S(  32,  14), S(  37,  36), S(  40,  37),
    S( -13,  16), S(  33,   9), S(  41,  36), S(  53,  34),
    S( -18, -13), S( -22,   8), S(  55, -21), S(  28,   1),
    S(-140, -24), S( -74, -15), S(-130,   3), S( -19, -10),
};
const int BishopPSQT32[32] = {
    S(  24, -25), S(  18, -27), S(  -3, -11), S(  12, -19),
    S(  33, -30), S(  31, -23), S(  24, -15), S(   7,  -5),
    S(  22, -14), S(  32, -12), S(  20,   0), S(  19,   4),
    S(  19,  -2), S(  12,   0), S(  11,  13), S(  29,  14),
    S( -12,   9), S(  22,   4), S(   6,  14), S(  28,  17),
    S(   0,   6), S(   5,   8), S(  29,  10), S(  22,   8),
    S( -47,   3), S(  -1,  -2), S(  -3,  -7), S( -28,   4),
    S( -47,  -3), S( -57,  -4), S(-112,   5), S(-102,   9),
};
const int RookPSQT32[32] = {
    S(  -3, -29), S(  -7, -18), S(   4, -15), S(   9, -19),
    S( -35, -25), S( -10, -28), S(  -2, -23), S(   4, -27),
    S( -21, -18), S(   0, -15), S(  -1, -17), S(  -1, -18),
    S( -15,   1), S(  -6,   6), S(  -1,   3), S(  -1,   1),
    S(  -6,  13), S(  -5,  11), S(  22,   8), S(  24,   9),
    S(  -9,  15), S(  19,   9), S(  17,  13), S(  24,  14),
    S(   2,  15), S(  -8,  13), S(  34,   1), S(  23,   9),
    S(   7,  22), S(  17,  15), S( -16,  21), S(   9,  28),
};
const int QueenPSQT32[32] = {
    S(   0, -43), S( -12, -32), S(  -9, -25), S(  13, -40),
    S(   2, -48), S(  10, -37), S(  19, -48), S(  13, -16),
    S(   5, -22), S(  22, -16), S(   7,   5), S(   4,   3),
    S(   6,  -5), S(   8,   4), S(  -2,  15), S(  -7,  41),
    S(  -9,  12), S( -10,  33), S(  -6,  23), S( -22,  48),
    S( -14,   6), S(  -3,  20), S(   0,  20), S(  -6,  42),
    S(   0,  17), S( -61,  54), S(  20,  11), S( -22,  58),
    S(  -5, -14), S(   7,  -9), S(  11,  -4), S( -14,   9),
};
const int KingPSQT32[32] = {
    S(  69, -96), S(  83, -70), S(  35, -32), S(  17, -36),
    S(  62, -47), S(  53, -39), S(   8,  -4), S( -19,   2),
    S(   0, -32), S(  41, -25), S(  17,   1), S( -12,  15),
    S( -42, -29), S(  37, -13), S(   4,  14), S( -49,  29),
    S( -17, -21), S(  59,   1), S(   6,  24), S( -46,  22),
    S(  31, -21), S(  84,  -3), S(  70,  14), S(  -2,   3),
    S(   1, -27), S(  56,  -6), S(  52,   3), S(  28,   1),
    S( -13,-102), S(  88, -60), S(  -1, -25), S(   1, -33),
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
