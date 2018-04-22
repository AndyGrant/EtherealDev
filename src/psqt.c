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
    S( -21,   7), S(  11,   8), S(  -6,   9), S(  -5,   0),
    S( -21,   3), S(  -4,   2), S(  -5,  -4), S(   1,  -7),
    S( -19,  11), S(  -3,  12), S(   3,  -5), S(   5, -16),
    S(  -6,  22), S(   9,  19), S(   5,   2), S(   7, -14),
    S(  17,  42), S(  33,  43), S(  33,  21), S(  31,  -7),
    S(   4,  46), S(   6,  55), S(  30,  29), S(  47,  10),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};

const int KnightPSQT32[32] = {
    S( -23, -18), S(  11, -14), S(  -4,  13), S(  24,  17),
    S(  25, -15), S(  11,  19), S(  30,   1), S(  33,  21),
    S(  13,  13), S(  32,  14), S(  29,  26), S(  41,  46),
    S(  22,  29), S(  47,  33), S(  46,  57), S(  59,  60),
    S(  39,  35), S(  55,  35), S(  54,  64), S(  76,  64),
    S(  -2,  27), S(  50,  27), S(  58,  59), S(  73,  53),
    S(  -4,  12), S(  -9,  39), S(  97,   2), S(  53,  33),
    S( -97, -15), S( -74,  -5), S(-101,  22), S(  12,   8),
};

const int BishopPSQT32[32] = {
    S(  28,  12), S(  37,  13), S(  10,  15), S(  31,  16),
    S(  41,   5), S(  46,   4), S(  37,  17), S(  19,  25),
    S(  37,  24), S(  42,  23), S(  34,  34), S(  30,  40),
    S(  25,  21), S(  29,  22), S(  21,  37), S(  48,  43),
    S(   5,  37), S(  30,  26), S(  30,  38), S(  55,  41),
    S(  13,  29), S(  22,  24), S(  54,  26), S(  41,  20),
    S( -30,  39), S(  31,  31), S(  21,  26), S(  -8,  33),
    S(  16,  24), S( -13,  24), S(-112,  36), S( -76,  47),
};

const int RookPSQT32[32] = {
    S(   6,  -1), S(   4,  13), S(  15,  16), S(  21,   7),
    S( -28,  12), S(   6,   6), S(  11,  11), S(  19,   4),
    S( -11,  14), S(  12,  23), S(  10,  15), S(  10,  12),
    S(  -9,  37), S(   3,  43), S(   5,  40), S(   5,  37),
    S(   0,  57), S(  13,  45), S(  28,  51), S(  37,  44),
    S(   6,  58), S(  36,  55), S(  34,  56), S(  44,  49),
    S(   6,  52), S(   8,  53), S(  45,  40), S(  29,  42),
    S(  36,  65), S(  39,  56), S(  -3,  69), S(  33,  71),
};

const int QueenPSQT32[32] = {
    S(  18,  30), S(  11,  37), S(  14,  46), S(  40,  25),
    S(  28,  27), S(  38,  32), S(  44,  16), S(  40,  45),
    S(  37,  51), S(  50,  53), S(  32,  77), S(  27,  68),
    S(  30,  75), S(  45,  75), S(  26,  89), S(  25, 115),
    S(  21, 105), S(  22, 115), S(  37,  95), S(  21, 127),
    S(  40,  87), S(  37,  97), S(  56,  91), S(  36, 127),
    S(  44,  93), S( -30, 123), S(  59,  93), S(  18, 149),
    S(  64,  40), S(  78,  58), S(  67,  70), S(  59,  81),
};

const int KingPSQT32[32] = {
    S(  78,-100), S(  83, -75), S(  28, -33), S(  11, -37),
    S(  67, -48), S(  53, -39), S(   6,  -2), S( -31,   7),
    S( -11, -40), S(  36, -27), S(   8,   1), S( -22,  19),
    S( -65, -36), S(  25, -20), S(   4,  19), S( -51,  40),
    S( -27, -20), S(  65,   2), S(  13,  32), S( -23,  38),
    S(  48, -14), S( 114,   3), S(  86,  22), S(  16,  12),
    S(  35,  -7), S(  83,   5), S(  61,   6), S(  65,   2),
    S(  23, -75), S( 115, -44), S(  32, -31), S(   6, -27),
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
