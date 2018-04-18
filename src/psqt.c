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
    S( -26,   0), S(  -3,   0), S(  -5,  -6), S(  -2, -10),
    S( -20,   7), S(  -6,   6), S(   3,  -9), S(   3, -22),
    S( -11,  14), S(   3,   8), S(   0,  -3), S(   4, -23),
    S(   2,  25), S(  13,  23), S(  15,   3), S(  17, -22),
    S( -50,   2), S( -42,   7), S(  -1, -17), S(   0, -35),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -40, -50), S(   3, -40), S( -13, -14), S(   9,  -8),
    S(   8, -52), S(   3, -12), S(  15, -25), S(  19,  -3),
    S(   4, -20), S(  26, -19), S(  15,   4), S(  29,  13),
    S(   5,   8), S(  28,   9), S(  31,  32), S(  40,  36),
    S(  25,   7), S(  36,  13), S(  39,  40), S(  50,  42),
    S( -27,   8), S(  30,   6), S(  43,  38), S(  52,  35),
    S( -42, -21), S( -43,   6), S(  48, -30), S(  14,  -4),
    S(-172, -32), S(-106, -32), S(-161, -11), S( -65, -28),
};

const int BishopPSQT32[32] = {
    S(  18, -22), S(  22, -24), S(   2,  -9), S(  19, -14),
    S(  34, -29), S(  32, -23), S(  24, -14), S(  10,  -2),
    S(  25, -11), S(  31, -12), S(  24,   0), S(  19,   7),
    S(  11,  -4), S(  14,   0), S(  10,  15), S(  36,  19),
    S( -10,  13), S(  25,   4), S(   8,  17), S(  36,  21),
    S(  -5,   9), S(   2,   8), S(  30,   7), S(  23,   5),
    S( -62,   4), S(   6,  -2), S(  -5, -12), S( -35,   2),
    S( -40,   0), S( -56,  -4), S(-143,   5), S(-123,  14),
};

const int RookPSQT32[32] = {
    S(  -5, -32), S(  -6, -17), S(   5, -13), S(  10, -20),
    S( -34, -26), S(  -6, -27), S(   3, -20), S(  10, -26),
    S( -19, -21), S(   5, -13), S(   1, -20), S(   3, -20),
    S( -20,  -1), S( -11,   4), S(  -4,   2), S(   2,   2),
    S( -12,  12), S( -10,   8), S(  20,   6), S(  23,   8),
    S( -16,  15), S(  17,  10), S(  18,  14), S(  26,  14),
    S(  -2,  16), S(  -7,  17), S(  41,   3), S(  24,   9),
    S(   0,  23), S(  16,  14), S( -19,  24), S(  12,  30),
};

const int QueenPSQT32[32] = {
    S(  -2, -47), S( -12, -28), S(  -5, -18), S(  14, -39),
    S(   5, -49), S(  12, -37), S(  19, -51), S(  15, -15),
    S(   6, -23), S(  22, -18), S(   8,   4), S(   5,   3),
    S(   5,  -5), S(  11,   4), S(   0,  13), S(  -2,  46),
    S(  -9,   9), S( -11,  33), S(  -4,  21), S( -19,  52),
    S( -10,   2), S(   1,  20), S(   5,  19), S(  -5,  47),
    S(   2,  13), S( -63,  55), S(  30,  12), S( -20,  69),
    S( -28, -31), S(   3, -17), S(   7, -10), S( -12,   9),
};

const int KingPSQT32[32] = {
    S(  80,-104), S(  89, -80), S(  37, -34), S(  21, -38),
    S(  71, -53), S(  60, -45), S(  11,  -5), S( -21,   2),
    S(  -3, -42), S(  44, -30), S(  18,  -1), S( -12,  15),
    S( -58, -36), S(  30, -21), S(   5,  15), S( -46,  36),
    S( -25, -18), S(  53,   1), S(   3,  30), S( -34,  38),
    S(  37, -16), S(  78,   0), S(  63,  18), S(  -2,  16),
    S(  24, -14), S(  47,  -5), S(  31,   0), S(   8,   1),
    S( -13, -90), S(  80, -67), S( -30, -39), S( -34, -39),
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
