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
    S( -22,   2), S(  11,   0), S(  -8,   5), S(  -4,   1),
    S( -26,  -2), S(  -9,  -3), S(  -7, -13), S(  -1, -16),
    S( -21,   5), S( -11,   4), S(   6, -18), S(   5, -30),
    S(  -8,  15), S(   1,  12), S(  -3,  -4), S(   6, -26),
    S(   3,  32), S(  22,  32), S(  20,  12), S(  34, -13),
    S( -40,  11), S( -36,  15), S(   0, -13), S(   1, -28),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -33, -41), S(   5, -30), S(  -6, -15), S(   7, -12),
    S(   7, -40), S(  13,  -8), S(  10, -25), S(  16,  -9),
    S(   6, -22), S(  20, -16), S(  10,  -5), S(  21,   5),
    S(  10,  -3), S(  21,   8), S(  23,  14), S(  27,  23),
    S(  30,   6), S(  24,  10), S(  26,  28), S(  35,  33),
    S( -26,  13), S(  20,  10), S(  26,  26), S(  40,  22),
    S( -13, -17), S( -24,  10), S(  70, -30), S(  28,  -6),
    S(-158, -29), S( -91, -22), S(-136,  -1), S( -40, -20),
};

const int BishopPSQT32[32] = {
    S(  23, -22), S(  21, -27), S(  -2,  -4), S(  12, -11),
    S(  35, -28), S(  30, -24), S(  23, -14), S(   9,  -6),
    S(  25, -18), S(  27, -15), S(  19,   3), S(  15,   6),
    S(  16, -11), S(   9,  -6), S(   9,   9), S(  30,  13),
    S( -13,   5), S(  21,   0), S(   4,  11), S(  23,  20),
    S(   1,  -1), S(  -4,   6), S(  24,   5), S(  21,   3),
    S( -56,   5), S(   9,  -4), S(   0, -13), S( -30,  -1),
    S( -43,   1), S( -56,  -1), S(-110,   8), S(-102,  11),
};

const int RookPSQT32[32] = {
    S(   2, -29), S(   1, -21), S(  12, -17), S(  18, -20),
    S( -29, -17), S(  -1, -22), S(   7, -17), S(  10, -18),
    S( -14, -11), S(  10, -17), S(  -4,  -6), S(   5, -16),
    S( -13,   0), S( -10,   1), S(  -3,   0), S(   0,   2),
    S(  -3,   8), S(  -9,  10), S(  14,   1), S(  15,   4),
    S( -12,  14), S(  12,   6), S(   4,  11), S(  21,   5),
    S(   0,   2), S( -13,   8), S(  26,  -6), S(  12,  -2),
    S(   1,  12), S(   6,  10), S( -17,  19), S(   2,  16),
};

const int QueenPSQT32[32] = {
    S(   5, -43), S(   2, -38), S(   7, -32), S(  25, -47),
    S(  13, -42), S(  23, -43), S(  29, -55), S(  24, -18),
    S(  12, -25), S(  30, -20), S(  14,   9), S(  13,   3),
    S(  16, -16), S(  13,   6), S(   1,  17), S(   0,  34),
    S( -14,   8), S(  -8,  24), S( -15,  25), S( -26,  58),
    S( -32,   4), S( -13,  16), S( -10,  19), S( -10,  36),
    S(  -2,   9), S( -44,  47), S(  12,  12), S( -36,  48),
    S( -40,  -4), S(  -2,  -6), S(   8,   7), S( -30,   1),
};

const int KingPSQT32[32] = {
    S(  73, -97), S(  80, -64), S(  40, -28), S(  26, -37),
    S(  65, -47), S(  54, -35), S(   7,   0), S( -17,   7),
    S(   0, -35), S(  46, -31), S(  14,   0), S( -22,  19),
    S( -50, -22), S(  31, -18), S(   0,  12), S( -44,  31),
    S( -19, -15), S(  48,  -4), S(   6,  24), S( -28,  30),
    S(  34, -13), S(  75,  -4), S(  65,  17), S(  12,  22),
    S(  13, -15), S(  44,  -5), S(  30,  -1), S(   5,  -1),
    S(  25, -71), S(  74, -62), S( -19, -29), S( -15, -33),
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
