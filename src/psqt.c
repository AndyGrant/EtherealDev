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
    S( -16,   6), S(  15,   1), S(  -1,   6), S( -10,  -4),
    S( -18,   2), S(   0,   1), S(  -6,  -4), S( -13, -14),
    S( -17,  16), S(   0,  11), S(  19,  -6), S(  25, -17),
    S(  -3,  21), S(   7,  10), S(   8,   3), S(  25, -14),
    S(   0,  29), S(  11,  26), S(  16,   4), S(  25, -19),
    S( -43,   7), S( -33,   9), S(  -1, -15), S(   1, -30),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -39, -48), S(  -9, -38), S( -12, -16), S(   1,  -8),
    S(   2, -49), S(   5, -11), S(   1, -23), S(   4,  -5),
    S(  -3, -23), S(  21, -19), S(  17,   0), S(  29,  12),
    S(   4,   5), S(  27,   7), S(  36,  32), S(  53,  34),
    S(  28,   5), S(  49,  13), S(  47,  42), S(  59,  46),
    S( -27,   8), S(  25,   4), S(  41,  37), S(  50,  34),
    S( -33, -21), S( -35,   4), S(  40, -32), S(  13,  -1),
    S(-168, -34), S(-102, -30), S(-156,  -6), S( -39, -26),
};
const int BishopPSQT32[32] = {
    S(  24, -22), S(  18, -27), S( -19, -12), S(  14, -16),
    S(  33, -29), S(  25, -26), S(  23, -13), S(   3,  -3),
    S(  21, -11), S(  30, -11), S(  19,   0), S(  31,   8),
    S(   8,  -4), S(  18,   0), S(  23,  14), S(  35,  19),
    S( -13,  11), S(  40,   4), S(   5,  17), S(  32,  20),
    S(   0,   6), S(   0,   7), S(  26,   7), S(  21,   7),
    S( -68,   1), S(  -2,  -3), S(  -8, -10), S( -39,   0),
    S( -48,   0), S( -61,  -1), S(-125,   3), S(-110,  10),
};
const int RookPSQT32[32] = {
    S( -12, -33), S( -12, -18), S(   9, -18), S(  18, -24),
    S( -43, -26), S(  -7, -29), S(  -2, -24), S(   3, -33),
    S( -18, -20), S(   4, -14), S(  -4, -21), S(   0, -22),
    S( -16,   0), S(  -9,   4), S(  -1,   2), S(   0,   4),
    S(  -9,  13), S(  -7,  11), S(  17,   6), S(  22,   8),
    S( -14,  15), S(  16,  10), S(  11,  14), S(  19,  13),
    S(   0,  16), S(  -6,  18), S(  34,   2), S(  20,   9),
    S(   0,  24), S(  11,  13), S( -21,  23), S(   3,  27),
};
const int QueenPSQT32[32] = {
    S(   0, -47), S( -14, -30), S(  -6, -21), S(   0, -41),
    S(   6, -47), S(  13, -37), S(  18, -54), S(   5, -15),
    S(   4, -22), S(  24, -16), S(   6,   5), S(   1,   0),
    S(   5,  -4), S(  11,   4), S(  -3,  15), S(  -2,  46),
    S(  -8,  10), S( -11,  33), S(  -5,  22), S( -19,  52),
    S( -11,   3), S(  -4,  18), S(   0,  21), S(  -8,  46),
    S(  -3,  12), S( -73,  55), S(  22,  11), S( -18,  66),
    S( -19, -21), S(   2, -11), S(   8,  -3), S( -17,   9),
};
const int KingPSQT32[32] = {
    S(  77,-108), S( 108, -79), S(  32, -36), S(  13, -42),
    S(  66, -55), S(  57, -40), S(   4,  -1), S( -17,   5),
    S(   0, -42), S(  44, -28), S(  16,   2), S( -11,  21),
    S( -53, -34), S(  33, -19), S(   1,  18), S( -44,  39),
    S( -18, -18), S(  54,   0), S(   8,  31), S( -29,  38),
    S(  37, -18), S(  83,   0), S(  74,  19), S(   9,  16),
    S(  14, -17), S(  49,  -4), S(  33,   0), S(   7,   0),
    S(  26, -81), S(  83, -67), S( -22, -35), S( -16, -36),
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
