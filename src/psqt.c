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
    S( -10,  14), S(  34,  11), S(  10,  11), S(   1,  -6),
    S( -16,  14), S(  15,  10), S(   1,   6), S(   0,  -8),
    S(   0,  21), S(  26,  19), S(  30,   2), S(  21, -15),
    S(  12,  22), S(  44,  19), S(  31,   5), S(  34,  -8),
    S(  24,  14), S(  25,  21), S(  43,  18), S(  43,  -8),
    S( -44, -30), S( -32,  -1), S(   0, -16), S(   5, -31),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -41, -51), S(  -5, -45), S( -18, -27), S(   2, -19),
    S(   1, -52), S(   4, -16), S(   1, -38), S(  15, -20),
    S(  -3, -30), S(  19, -26), S(   8, -10), S(  31,   2),
    S(   8,   2), S(  28,   8), S(  28,  27), S(  44,  31),
    S(  29,   8), S(  43,  16), S(  44,  42), S(  56,  44),
    S( -27,  13), S(  29,   8), S(  43,  41), S(  55,  41),
    S( -35, -21), S( -35,   5), S(  46, -29), S(  14,  -4),
    S(-168, -36), S(-102, -30), S(-158,  -7), S( -40, -24),
};
const int BishopPSQT32[32] = {
    S(  26, -29), S(  20, -33), S( -13, -21), S(  12, -30),
    S(  36, -35), S(  33, -34), S(  28, -21), S(   5,  -9),
    S(  17, -18), S(  41, -18), S(  20,  -4), S(  20,   0),
    S(  19,  -1), S(  15,   0), S(  11,  16), S(  35,  12),
    S( -20,   9), S(  21,   7), S(   6,  15), S(  39,  18),
    S(  -3,   4), S(   0,   7), S(  29,  16), S(  21,  14),
    S( -66,   2), S(  -3,  -2), S(  -9,  -8), S( -39,   7),
    S( -51,  -2), S( -65,  -5), S(-128,   5), S(-113,  11),
};
const int RookPSQT32[32] = {
    S(  -6, -37), S( -12, -25), S(   5, -29), S(  10, -35),
    S( -48, -40), S( -15, -37), S(  -6, -38), S(   0, -44),
    S( -28, -26), S(   0, -18), S(   0, -22), S(  -6, -23),
    S( -16,   1), S(  -4,  13), S(   1,   9), S(   1,   5),
    S(  -4,  20), S(  -5,  20), S(  31,  21), S(  36,  27),
    S( -13,  17), S(  21,  13), S(  20,  22), S(  29,  28),
    S(   1,  21), S( -12,  13), S(  37,   1), S(  25,  16),
    S(   5,  34), S(  15,  21), S( -22,  22), S(   8,  39),
};
const int QueenPSQT32[32] = {
    S(  -3, -48), S( -21, -34), S( -22, -27), S(   4, -49),
    S(  -3, -53), S(   7, -42), S(  14, -59), S(   7, -23),
    S(   4, -25), S(  24, -19), S(   7,   5), S(   0,   2),
    S(   6,  -4), S(  14,   5), S(   1,  19), S(   1,  50),
    S(  -5,  14), S(  -4,  37), S(   2,  29), S( -16,  56),
    S( -12,   5), S(  -5,  19), S(   4,  24), S(  -5,  49),
    S(  -4,  14), S( -78,  53), S(  23,  10), S( -18,  67),
    S( -19, -23), S(   1, -15), S(   7,  -6), S( -19,   8),
};
const int KingPSQT32[32] = {
    S(  78,-102), S( 104, -75), S(  38, -34), S(  14, -32),
    S(  71, -44), S(  45, -31), S(   5,   9), S( -23,  22),
    S(  -2, -30), S(  45, -20), S(  21,  13), S(  -9,  39),
    S( -51, -35), S(  37,  -9), S(   6,  23), S( -47,  42),
    S( -19, -26), S(  60,   1), S(   9,  22), S( -38,   6),
    S(  38, -24), S(  85, -11), S(  75,   7), S(   6, -10),
    S(  15, -24), S(  51, -13), S(  36,  -4), S(  11,  -2),
    S(  27, -86), S(  85, -70), S( -22, -37), S( -15, -38),
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
