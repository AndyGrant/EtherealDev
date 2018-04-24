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
    S( -23,   2), S(  12,   3), S(  -5,   6), S(  -5,   0),
    S( -27,   0), S(  -4,   0), S(  -6,  -6), S(  -2, -11),
    S( -21,   7), S(  -7,   6), S(   4,  -9), S(   2, -23),
    S( -12,  14), S(   2,   9), S(  -1,  -3), S(   3, -23),
    S(   0,  26), S(  11,  25), S(  16,   4), S(  20, -23),
    S( -53,   2), S( -34,   6), S(  -4, -19), S(   0, -36),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -44, -52), S(   3, -40), S( -14, -17), S(  10,  -8),
    S(   8, -53), S(   5, -13), S(  15, -26), S(  19,  -4),
    S(   4, -22), S(  26, -19), S(  15,   4), S(  30,  12),
    S(   4,   7), S(  30,   9), S(  31,  32), S(  40,  35),
    S(  26,   5), S(  37,  13), S(  39,  40), S(  49,  42),
    S( -29,   7), S(  30,   5), S(  45,  38), S(  53,  35),
    S( -42, -26), S( -43,   5), S(  45, -34), S(  14,  -5),
    S(-174, -35), S(-100, -32), S(-162, -12), S( -50, -29),
};

const int BishopPSQT32[32] = {
    S(  22, -24), S(  24, -27), S(   2,  -9), S(  20, -14),
    S(  36, -31), S(  33, -24), S(  25, -15), S(  10,  -2),
    S(  25, -11), S(  31, -14), S(  24,   0), S(  20,   7),
    S(  11,  -5), S(  13,  -1), S(  11,  14), S(  34,  19),
    S( -12,  12), S(  26,   4), S(   5,  17), S(  33,  20),
    S(  -6,   7), S(   0,   7), S(  28,   7), S(  22,   5),
    S( -67,   0), S(   0,  -4), S( -10, -14), S( -40,   0),
    S( -54,   0), S( -67,  -4), S(-136,   4), S(-119,  11),
};

const int RookPSQT32[32] = {
    S(  -5, -33), S(  -7, -18), S(   5, -14), S(  11, -20),
    S( -36, -27), S(  -6, -29), S(   2, -20), S(  11, -26),
    S( -22, -22), S(   5, -14), S(   0, -19), S(   3, -21),
    S( -21,  -2), S( -10,   4), S(  -2,   2), S(   0,   2),
    S( -16,  11), S( -14,   8), S(  19,   6), S(  23,   7),
    S( -20,  13), S(  17,   8), S(  16,  14), S(  22,  15),
    S(  -4,  17), S(  -8,  16), S(  39,   3), S(  22,  10),
    S(   1,  23), S(  16,  14), S( -24,  23), S(  11,  31),
};

const int QueenPSQT32[32] = {
    S(  -3, -49), S( -11, -31), S(  -5, -21), S(  14, -41),
    S(   5, -52), S(  13, -38), S(  20, -52), S(  15, -16),
    S(   6, -25), S(  23, -18), S(   8,   5), S(   5,   3),
    S(   5,  -6), S(  11,   4), S(  -2,  15), S(  -3,  47),
    S( -10,   9), S( -13,  34), S(  -4,  21), S( -20,  52),
    S( -11,   3), S(  -2,  19), S(   2,  20), S(  -5,  48),
    S(  -3,  13), S( -75,  56), S(  30,  11), S( -17,  70),
    S( -24, -28), S(   5, -16), S(   8,  -9), S( -16,   9),
};

const int KingPSQT32[32] = {
    S(  83,-107), S(  91, -81), S(  39, -35), S(  22, -40),
    S(  73, -54), S(  60, -46), S(  11,  -4), S( -23,   3),
    S(  -5, -43), S(  45, -32), S(  16,   0), S( -15,  17),
    S( -63, -38), S(  29, -21), S(   2,  14), S( -48,  38),
    S( -28, -19), S(  52,   1), S(   2,  31), S( -36,  39),
    S(  37, -18), S(  78,  -1), S(  68,  18), S(  -2,  15),
    S(  16, -17), S(  41,  -7), S(  22,  -4), S(   0,  -2),
    S(  -2, -93), S(  81, -72), S( -43, -41), S( -34, -42),
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
