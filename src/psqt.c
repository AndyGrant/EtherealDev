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
    S( -27,   6), S(  10,   5), S( -10,   5), S( -11,  -4),
    S( -24,   2), S(   0,   0), S(  -9,  -3), S( -12, -13),
    S( -18,  10), S(   2,   9), S(  10,  -7), S(   7, -22),
    S(  -6,  14), S(  13,  11), S(   7,  -1), S(  13, -17),
    S(   2,  20), S(  13,  23), S(  21,   7), S(  26, -19),
    S( -46,  -2), S( -34,   7), S(  -3, -16), S(   1, -33),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -40, -49), S(   1, -40), S( -10, -17), S(   9, -10),
    S(   6, -50), S(   9, -11), S(  11, -27), S(  20,  -8),
    S(   1, -22), S(  25, -19), S(   9,   0), S(  29,  11),
    S(  11,   6), S(  27,   8), S(  29,  29), S(  35,  32),
    S(  28,   6), S(  35,  14), S(  38,  38), S(  43,  40),
    S( -28,   9), S(  28,   5), S(  39,  37), S(  49,  34),
    S( -36, -23), S( -36,   5), S(  44, -31), S(  13,  -2),
    S(-169, -36), S(-102, -30), S(-158,  -8), S( -41, -26),
};
const int BishopPSQT32[32] = {
    S(  26, -22), S(  22, -27), S( -11, -12), S(  17, -18),
    S(  36, -29), S(  32, -26), S(  26, -15), S(   6,  -6),
    S(  24, -13), S(  34, -12), S(  22,   0), S(  20,   4),
    S(  15,  -4), S(  13,   0), S(  13,  14), S(  31,  14),
    S( -14,  10), S(  26,   4), S(   4,  15), S(  29,  18),
    S(  -4,   5), S(   0,   7), S(  25,   8), S(  19,   7),
    S( -67,   2), S(  -3,  -4), S( -11, -11), S( -41,   1),
    S( -51,  -1), S( -64,  -3), S(-128,   6), S(-113,  11),
};
const int RookPSQT32[32] = {
    S(  -5, -32), S(  -9, -20), S(   3, -20), S(   9, -25),
    S( -37, -27), S(  -8, -29), S(  -1, -25), S(   5, -31),
    S( -21, -20), S(   1, -15), S(  -1, -19), S(   0, -21),
    S( -19,   0), S( -10,   6), S(  -2,   3), S(  -1,   1),
    S( -11,  14), S( -10,  11), S(  19,   8), S(  21,   9),
    S( -16,  15), S(  16,   9), S(  12,  14), S(  19,  14),
    S(  -1,  17), S(  -9,  14), S(  35,   1), S(  21,   9),
    S(   1,  23), S(  11,  14), S( -23,  22), S(   3,  28),
};
const int QueenPSQT32[32] = {
    S(   0, -47), S( -11, -30), S(  -6, -22), S(  10, -43),
    S(   6, -49), S(  13, -38), S(  20, -53), S(  11, -17),
    S(   6, -23), S(  25, -18), S(   7,   5), S(   3,   3),
    S(   7,  -5), S(   8,   4), S(  -3,  15), S(  -8,  45),
    S( -11,  10), S( -13,  33), S(  -7,  22), S( -24,  52),
    S( -14,   3), S(  -5,  19), S(   0,  21), S( -10,  46),
    S(  -6,  12), S( -75,  55), S(  22,  10), S( -21,  66),
    S( -21, -23), S(   2, -13), S(   8,  -5), S( -19,   9),
};
const int KingPSQT32[32] = {
    S(  79,-104), S(  95, -74), S(  40, -35), S(  18, -39),
    S(  70, -52), S(  55, -43), S(   9,  -3), S( -20,   5),
    S(   0, -37), S(  44, -27), S(  17,   3), S( -14,  19),
    S( -52, -32), S(  34, -15), S(   1,  15), S( -48,  30),
    S( -19, -19), S(  56,   2), S(   7,  27), S( -34,  26),
    S(  39, -18), S(  85,  -1), S(  74,  18), S(   8,  12),
    S(  16, -17), S(  52,  -4), S(  35,   0), S(   9,   1),
    S(  28, -81), S(  86, -66), S( -21, -34), S( -15, -35),
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
