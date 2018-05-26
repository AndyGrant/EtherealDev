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
    S( -23,   1), S(  14,   2), S(  -8,   7), S(  -3,   1),
    S( -27,  -1), S(  -8,  -2), S(  -8, -11), S(  -2, -14),
    S( -21,   7), S(  -9,   5), S(   8, -17), S(   6, -29),
    S(  -7,  17), S(   2,  12), S(  -1,  -3), S(   8, -28),
    S(   2,  34), S(  18,  33), S(  19,  10), S(  28, -17),
    S( -45,   9), S( -35,  12), S(  -3, -14), S(   1, -31),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};
const int KnightPSQT32[32] = {
    S( -39, -48), S(   4, -37), S(  -9, -15), S(   7, -10),
    S(   7, -48), S(  12, -10), S(  13, -27), S(  19,  -7),
    S(   9, -22), S(  24, -19), S(  13,  -1), S(  26,  11),
    S(  10,   2), S(  26,   8), S(  29,  24), S(  34,  32),
    S(  31,   7), S(  29,  13), S(  38,  37), S(  42,  42),
    S( -28,  10), S(  27,   7), S(  38,  36), S(  49,  33),
    S( -32, -22), S( -34,   6), S(  54, -29), S(  16,  -3),
    S(-169, -36), S(-103, -30), S(-157,  -7), S( -42, -27),
};
const int BishopPSQT32[32] = {
    S(  27, -23), S(  23, -27), S(   0,  -5), S(  18, -15),
    S(  38, -29), S(  35, -25), S(  27, -16), S(  11,  -6),
    S(  27, -15), S(  32, -14), S(  22,   2), S(  18,   7),
    S(  13,  -7), S(  10,  -3), S(  11,  12), S(  33,  17),
    S( -13,   9), S(  19,   2), S(   4,  15), S(  26,  22),
    S(   0,   5), S(  -2,   7), S(  25,   7), S(  21,   6),
    S( -64,   3), S(   2,  -3), S(  -7, -12), S( -40,  -1),
    S( -50,   0), S( -64,  -3), S(-127,   6), S(-114,  11),
};
const int RookPSQT32[32] = {
    S(   1, -29), S(   0, -20), S(  11, -15), S(  17, -19),
    S( -34, -23), S(  -4, -26), S(   4, -17), S(  10, -22),
    S( -17, -15), S(   6, -14), S(  -2, -14), S(   3, -18),
    S( -18,   0), S( -13,   2), S(  -5,   0), S(  -1,   2),
    S( -10,  11), S( -12,   9), S(  15,   2), S(  18,   4),
    S( -17,  14), S(  13,   8), S(   7,  11), S(  18,  10),
    S(  -2,  12), S( -10,  14), S(  34,   0), S(  18,   5),
    S(   0,  19), S(   9,  12), S( -23,  22), S(   1,  23),
};
const int QueenPSQT32[32] = {
    S(   0, -47), S(  -4, -30), S(   1, -22), S(  23, -40),
    S(   9, -48), S(  18, -38), S(  27, -51), S(  21, -13),
    S(   8, -23), S(  28, -17), S(  10,   8), S(   8,   5),
    S(  10,  -7), S(  11,   6), S(  -6,  16), S(  -6,  44),
    S( -19,   9), S( -16,  32), S( -15,  20), S( -31,  53),
    S( -27,   0), S( -11,  17), S(  -8,  19), S( -14,  43),
    S(  -6,  12), S( -66,  57), S(  19,  10), S( -26,  62),
    S( -25, -20), S(   1, -12), S(   8,  -2), S( -23,   6),
};
const int KingPSQT32[32] = {
    S(  79,-109), S(  89, -77), S(  42, -34), S(  22, -42),
    S(  70, -55), S(  61, -43), S(   7,  -2), S( -20,   6),
    S(   0, -41), S(  45, -32), S(  16,   0), S( -16,  17),
    S( -52, -29), S(  33, -19), S(   0,  14), S( -47,  35),
    S( -19, -17), S(  55,   0), S(   7,  30), S( -32,  36),
    S(  39, -16), S(  85,   0), S(  74,  20), S(  10,  20),
    S(  16, -17), S(  51,  -3), S(  34,   0), S(   8,   0),
    S(  29, -81), S(  85, -67), S( -21, -34), S( -16, -36),
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
