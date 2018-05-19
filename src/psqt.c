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
    S( -21,   4), S(  11,   4), S(  -7,   7), S(  -5,  -2),
    S( -22,   1), S(  -2,   0), S(  -7,  -6), S(  -4, -10),
    S( -20,   6), S(  -4,   6), S(   7,  -8), S(   5, -19),
    S(  -9,  12), S(   5,   8), S(   3,  -1), S(   7, -19),
    S(   0,  13), S(  18,  20), S(  22,   2), S(  29, -18),
    S( -48, -25), S( -33,  -1), S(   3, -19), S(   6, -36),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -27, -40), S(   4, -38), S( -15, -22), S(   4, -15),
    S(   1, -53), S(   8, -13), S(  10, -27), S(  17,  -8),
    S(   2, -22), S(  23, -19), S(  11,   0), S(  25,  11),
    S(  11,   8), S(  26,   8), S(  27,  29), S(  32,  30),
    S(  32,  12), S(  31,  12), S(  39,  36), S(  43,  37),
    S( -14,  15), S(  38,  10), S(  44,  38), S(  50,  32),
    S( -19, -12), S( -24,   9), S(  53, -23), S(  29,   1),
    S(-150, -23), S( -83, -19), S(-129,   0), S( -26, -19),
};

const int BishopPSQT32[32] = {
    S(  27, -25), S(  22, -32), S(  -2, -11), S(  11, -17),
    S(  37, -27), S(  32, -24), S(  26, -19), S(   8,  -5),
    S(  24, -12), S(  30, -11), S(  22,   0), S(  19,   3),
    S(  19,  -3), S(  10,  -2), S(   9,  12), S(  31,  11),
    S(  -7,  12), S(  21,   4), S(  10,  11), S(  28,  17),
    S(  -2,   8), S(  10,   6), S(  30,   8), S(  23,  10),
    S( -52,   5), S(  -1,  -5), S(  -1,  -5), S( -29,   5),
    S( -41,  -3), S( -51,   1), S(-116,   6), S( -95,  13),
};

const int RookPSQT32[32] = {
    S(  -3, -30), S(  -7, -19), S(   3, -15), S(  10, -20),
    S( -37, -27), S( -11, -31), S(  -2, -24), S(   5, -27),
    S( -20, -17), S(   0, -15), S(  -3, -18), S(  -2, -21),
    S( -15,   2), S(  -8,   5), S(  -2,   3), S(   0,   2),
    S(  -5,  13), S(  -4,  11), S(  22,   8), S(  22,   6),
    S(  -6,  16), S(  20,   9), S(  16,  12), S(  26,  14),
    S(   2,  15), S(  -4,  15), S(  36,   1), S(  25,   7),
    S(   5,  19), S(  18,  14), S( -14,  23), S(   9,  25),
};

const int QueenPSQT32[32] = {
    S(   0, -47), S( -11, -32), S(  -7, -27), S(  13, -41),
    S(   6, -49), S(  12, -38), S(  22, -51), S(  13, -15),
    S(   5, -22), S(  22, -16), S(   9,   6), S(   4,   3),
    S(   7,  -2), S(   9,   5), S(  -4,  14), S(  -8,  42),
    S( -10,  12), S( -13,  32), S(  -6,  21), S( -23,  49),
    S( -13,   5), S(  -1,  21), S(  -2,  20), S( -10,  42),
    S(   1,  16), S( -58,  58), S(  20,  11), S( -23,  58),
    S(  -7, -15), S(   6, -13), S(  13,  -4), S( -17,   6),
};

const int KingPSQT32[32] = {
    S(  71, -86), S(  86, -69), S(  38, -31), S(  18, -37),
    S(  64, -45), S(  54, -39), S(   9,  -4), S( -20,   1),
    S(   0, -37), S(  40, -28), S(  16,   0), S( -13,  15),
    S( -42, -27), S(  34, -18), S(   0,  10), S( -48,  30),
    S( -12, -25), S(  48, -15), S(   0,  18), S( -42,  23),
    S(  42, -17), S(  82, -14), S(  65,  10), S(   3,   4),
    S(  18, -22), S(  57,  -4), S(  39,   2), S(  19,   0),
    S(  -3, -99), S( 111, -40), S(   4, -19), S(   6, -27),
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
