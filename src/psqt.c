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
    S( -23,   1), S(  12,   3), S(  -5,   6), S(  -6,  -1),
    S( -25,   0), S(  -3,  -1), S(  -5,  -6), S(  -2, -10),
    S( -21,   7), S(  -6,   6), S(   3,  -9), S(   3, -22),
    S( -11,  14), S(   3,   8), S(  -1,  -3), S(   4, -23),
    S(   2,  25), S(  13,  23), S(  16,   3), S(  16, -23),
    S( -45,   4), S( -38,   8), S(   0, -17), S(   1, -34),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -36, -47), S(   3, -40), S( -12, -13), S(   9,  -8),
    S(   9, -51), S(   3, -12), S(  16, -24), S(  20,  -3),
    S(   4, -19), S(  28, -18), S(  15,   4), S(  29,  14),
    S(   5,   9), S(  27,   9), S(  31,  33), S(  39,  37),
    S(  26,   8), S(  36,  14), S(  37,  41), S(  49,  42),
    S( -25,   8), S(  34,   6), S(  42,  38), S(  51,  35),
    S( -45, -19), S( -42,   6), S(  55, -28), S(  14,  -2),
    S(-169, -31), S(-108, -31), S(-159,  -8), S( -59, -25),
};

const int BishopPSQT32[32] = {
    S(  17, -21), S(  21, -22), S(   2,  -9), S(  19, -14),
    S(  32, -28), S(  33, -23), S(  24, -14), S(  10,  -2),
    S(  25, -11), S(  32, -12), S(  25,   0), S(  19,   7),
    S(  10,  -3), S(  13,   0), S(  10,  15), S(  34,  20),
    S(  -9,  13), S(  24,   4), S(   9,  17), S(  35,  21),
    S(  -6,   9), S(   5,   8), S(  31,   7), S(  22,   5),
    S( -59,   5), S(  10,  -2), S(  -3, -11), S( -35,   2),
    S( -38,  -1), S( -55,  -3), S(-144,   6), S(-125,  15),
};

const int RookPSQT32[32] = {
    S(  -4, -32), S(  -6, -16), S(   5, -13), S(  10, -20),
    S( -33, -25), S(  -6, -26), S(   1, -19), S(  10, -26),
    S( -20, -21), S(   3, -13), S(   1, -20), S(   1, -20),
    S( -21,  -1), S( -12,   4), S(  -6,   3), S(   0,   2),
    S( -13,  13), S(  -8,   9), S(  20,   7), S(  22,   8),
    S( -15,  16), S(  17,  11), S(  19,  15), S(  24,  14),
    S(  -1,  17), S(  -5,  17), S(  41,   3), S(  23,   9),
    S(  -1,  24), S(  16,  14), S( -19,  24), S(  11,  30),
};

const int QueenPSQT32[32] = {
    S(  -2, -45), S( -12, -27), S(  -5, -17), S(  14, -39),
    S(   3, -49), S(  12, -36), S(  19, -50), S(  14, -15),
    S(   6, -22), S(  22, -19), S(   7,   3), S(   4,   3),
    S(   5,  -5), S(   8,   3), S(   0,  12), S(  -3,  46),
    S(  -9,   9), S( -11,  32), S(  -4,  20), S( -19,  51),
    S(  -7,   2), S(   2,  20), S(   6,  18), S(  -6,  46),
    S(   4,  14), S( -59,  54), S(  31,  12), S( -17,  70),
    S( -22, -31), S(   4, -17), S(   3, -12), S( -11,   9),
};

const int KingPSQT32[32] = {
    S(  80,-104), S(  88, -79), S(  37, -35), S(  20, -37),
    S(  70, -53), S(  60, -45), S(  12,  -5), S( -18,   1),
    S(  -1, -41), S(  45, -29), S(  19,  -1), S( -11,  14),
    S( -54, -35), S(  29, -21), S(   6,  15), S( -46,  36),
    S( -26, -18), S(  53,   1), S(   5,  30), S( -34,  37),
    S(  38, -17), S(  81,   1), S(  63,  18), S(  -2,  17),
    S(  25, -14), S(  54,  -3), S(  35,   1), S(  16,   4),
    S(  -6, -84), S(  89, -62), S( -19, -36), S( -26, -34),
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
