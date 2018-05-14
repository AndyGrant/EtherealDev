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

#include "evaluate.h"
#include "piece.h"
#include "psqt.h"
#include "square.h"

int PSQT[32][SQUARE_NB];

// Undefined after all PSQT terms
#define S(mg, eg) (MakeScore((mg), (eg)))

const int PawnPSQT32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S( -22,   2), S(  13,   2), S(  -6,   5), S(  -4,   0),
    S( -26,   0), S(  -4,   0), S(  -7,  -6), S(  -1, -11),
    S( -20,   7), S(  -7,   6), S(   5, -10), S(   4, -23),
    S(  -9,  15), S(   3,  10), S(   0,  -2), S(   5, -23),
    S(   0,  30), S(  15,  29), S(  18,   8), S(  28, -18),
    S( -45,   9), S( -51,  13), S(  -2, -14), S(   2, -31),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -37, -47), S(   4, -37), S(  -7, -16), S(  10,  -8),
    S(   7, -49), S(  13, -10), S(  14, -26), S(  21,  -4),
    S(   6, -22), S(  25, -18), S(  14,   2), S(  28,  10),
    S(   8,   4), S(  28,   8), S(  30,  28), S(  35,  33),
    S(  29,   5), S(  31,  12), S(  39,  37), S(  43,  40),
    S( -32,  12), S(  22,   7), S(  36,  35), S(  47,  32),
    S( -26, -25), S( -31,   6), S(  40, -34), S(  17,  -7),
    S(-168, -36), S(-109, -30), S(-161, -10), S( -54, -29),
};

const int BishopPSQT32[32] = {
    S(  29, -23), S(  23, -31), S(   0,  -7), S(  18, -14),
    S(  39, -29), S(  33, -24), S(  26, -15), S(  11,  -3),
    S(  27, -13), S(  31, -13), S(  23,   1), S(  18,   6),
    S(  16,  -7), S(  14,  -2), S(  11,  13), S(  35,  17),
    S( -13,  10), S(  27,   3), S(   5,  15), S(  29,  19),
    S(  -3,   5), S(  -5,   8), S(  24,   6), S(  21,   5),
    S( -73,   3), S(   0,  -3), S(  -9, -15), S( -37,  -1),
    S( -41,   1), S( -65,  -4), S(-121,   6), S(-115,  10),
};

const int RookPSQT32[32] = {
    S(  -4, -31), S(  -5, -19), S(   5, -14), S(  12, -20),
    S( -35, -24), S(  -5, -28), S(   3, -19), S(  10, -25),
    S( -19, -17), S(   3, -14), S(  -3, -15), S(   3, -19),
    S( -18,   0), S( -12,   3), S(  -4,   1), S(   0,   2),
    S(  -9,  10), S( -12,   9), S(  16,   3), S(  20,   6),
    S( -15,  13), S(  14,   7), S(   9,  13), S(  23,  12),
    S(  -3,  13), S( -12,  15), S(  34,   1), S(  21,   8),
    S(   1,  19), S(   7,  13), S( -21,  22), S(  -1,  26),
};

const int QueenPSQT32[32] = {
    S(  -1, -48), S(  -7, -33), S(  -1, -23), S(  16, -41),
    S(  10, -47), S(  16, -38), S(  22, -52), S(  17, -14),
    S(   7, -24), S(  24, -17), S(   9,   7), S(   6,   4),
    S(   8,  -7), S(  12,   4), S(  -4,  17), S(  -4,  45),
    S( -21,   8), S( -15,  31), S( -11,  21), S( -25,  54),
    S( -33,   1), S( -12,  19), S(  -8,  21), S( -13,  45),
    S(  -8,  10), S( -73,  55), S(  15,  11), S( -34,  63),
    S( -44, -22), S(  -6, -15), S(   8,  -3), S( -39,   4),
};

const int KingPSQT32[32] = {
    S(  79,-107), S(  87, -80), S(  40, -33), S(  21, -39),
    S(  68, -55), S(  59, -44), S(  10,  -3), S( -23,   5),
    S(   0, -41), S(  48, -31), S(  20,   0), S( -11,  17),
    S( -53, -31), S(  39, -17), S(   5,  15), S( -39,  38),
    S( -14, -17), S(  59,   2), S(  10,  31), S( -26,  38),
    S(  38, -16), S(  86,  -1), S(  77,  20), S(  20,  19),
    S(   3, -16), S(  37,  -6), S(  28,  -3), S(  -4,  -2),
    S(  19, -86), S(  64, -71), S( -41, -37), S( -22, -41),
};

#undef S // Undefine MakeScore

void initializePSQT(){

    int sq, w32, b32;

    for (sq = 0; sq < SQUARE_NB; sq++){

        w32 = relativeSquare32(sq, WHITE);
        b32 = relativeSquare32(sq, BLACK);

        PSQT[WHITE_PAWN  ][sq] = +MakeScore(PieceValues[PAWN  ][MG], PieceValues[PAWN  ][EG]) +   PawnPSQT32[w32];
        PSQT[WHITE_KNIGHT][sq] = +MakeScore(PieceValues[KNIGHT][MG], PieceValues[KNIGHT][EG]) + KnightPSQT32[w32];
        PSQT[WHITE_BISHOP][sq] = +MakeScore(PieceValues[BISHOP][MG], PieceValues[BISHOP][EG]) + BishopPSQT32[w32];
        PSQT[WHITE_ROOK  ][sq] = +MakeScore(PieceValues[ROOK  ][MG], PieceValues[ROOK  ][EG]) +   RookPSQT32[w32];
        PSQT[WHITE_QUEEN ][sq] = +MakeScore(PieceValues[QUEEN ][MG], PieceValues[QUEEN ][EG]) +  QueenPSQT32[w32];
        PSQT[WHITE_KING  ][sq] = +MakeScore(PieceValues[KING  ][MG], PieceValues[KING  ][EG]) +   KingPSQT32[w32];

        PSQT[BLACK_PAWN  ][sq] = -MakeScore(PieceValues[PAWN  ][MG], PieceValues[PAWN  ][EG]) -   PawnPSQT32[b32];
        PSQT[BLACK_KNIGHT][sq] = -MakeScore(PieceValues[KNIGHT][MG], PieceValues[KNIGHT][EG]) - KnightPSQT32[b32];
        PSQT[BLACK_BISHOP][sq] = -MakeScore(PieceValues[BISHOP][MG], PieceValues[BISHOP][EG]) - BishopPSQT32[b32];
        PSQT[BLACK_ROOK  ][sq] = -MakeScore(PieceValues[ROOK  ][MG], PieceValues[ROOK  ][EG]) -   RookPSQT32[b32];
        PSQT[BLACK_QUEEN ][sq] = -MakeScore(PieceValues[QUEEN ][MG], PieceValues[QUEEN ][EG]) -  QueenPSQT32[b32];
        PSQT[BLACK_KING  ][sq] = -MakeScore(PieceValues[KING  ][MG], PieceValues[KING  ][EG]) -   KingPSQT32[b32];
    }
}
