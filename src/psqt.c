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

int PSQTMidgame[32][SQUARE_NB];
int PSQTEndgame[32][SQUARE_NB];

const int PawnPSQT32[32][PHASE_NB] = {
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
    { -23,   1}, {  12,   3}, {  -6,   5}, {  -5,   0},
    { -25,   0}, {  -3,  -1}, {  -6,  -6}, {  -2, -10},
    { -21,   7}, {  -6,   6}, {   4,  -9}, {   3, -22},
    { -11,  14}, {   3,   9}, {  -1,  -2}, {   5, -23},
    {   2,  26}, {  14,  24}, {  16,   4}, {  19, -21},
    { -46,   5}, { -46,   9}, {  -4, -16}, {   0, -34},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -37, -48}, {   4, -39}, { -10, -14}, {  10,  -8},
    {   8, -51}, {   6, -11}, {  15, -25}, {  20,  -3},
    {   5, -20}, {  27, -18}, {  15,   4}, {  29,  13},
    {   6,   8}, {  27,   9}, {  30,  32}, {  37,  36},
    {  27,   7}, {  34,  14}, {  38,  40}, {  47,  41},
    { -26,   9}, {  30,   6}, {  40,  37}, {  50,  35},
    { -41, -21}, { -40,   7}, {  50, -29}, {  15,  -4},
    {-173, -32}, {-112, -32}, {-162, -10}, { -63, -27},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {  21, -21}, {  22, -24}, {   2,  -9}, {  19, -14},
    {  34, -29}, {  33, -23}, {  24, -14}, {  11,  -2},
    {  25, -11}, {  32, -12}, {  24,   0}, {  18,   7},
    {  12,  -4}, {  13,  -1}, {  10,  14}, {  36,  19},
    { -11,  13}, {  24,   4}, {   9,  16}, {  34,  20},
    {  -5,   9}, {   1,   9}, {  30,   7}, {  22,   5},
    { -62,   5}, {   8,  -2}, {  -4, -11}, { -34,   1},
    { -38,   0}, { -56,  -3}, {-142,   7}, {-128,  14},
};

const int RookPSQT32[32][PHASE_NB] = {
    {  -4, -32}, {  -6, -17}, {   5, -14}, {  10, -20},
    { -34, -25}, {  -5, -26}, {   2, -19}, {  10, -25},
    { -18, -20}, {   4, -13}, {   0, -19}, {   2, -20},
    { -19,   0}, { -12,   4}, {  -5,   3}, {   0,   2},
    { -11,  13}, {  -8,   9}, {  19,   6}, {  22,   8},
    { -14,  15}, {  16,  10}, {  18,  14}, {  26,  14},
    {  -1,  16}, {  -7,  17}, {  40,   3}, {  23,   9},
    {  -1,  23}, {  13,  14}, { -19,  23}, {   7,  29},
};

const int QueenPSQT32[32][PHASE_NB] = {
    {  -1, -46}, { -11, -28}, {  -4, -19}, {  15, -40},
    {   6, -48}, {  13, -36}, {  20, -50}, {  15, -15},
    {   6, -23}, {  22, -18}, {   7,   5}, {   4,   3},
    {   6,  -5}, {  10,   4}, {  -2,  13}, {  -4,  46},
    { -11,   9}, { -13,  32}, {  -7,  21}, { -21,  52},
    { -12,   2}, {   0,  20}, {   2,  20}, {  -9,  46},
    {   0,  13}, { -62,  55}, {  28,  12}, { -24,  69},
    { -29, -29}, {   3, -16}, {   6,  -9}, { -15,   9},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  80,-105}, {  88, -80}, {  38, -35}, {  21, -38},
    {  70, -54}, {  60, -45}, {  11,  -5}, { -20,   2},
    {  -1, -41}, {  45, -30}, {  17,   0}, { -12,  15},
    { -52, -34}, {  33, -19}, {   6,  15}, { -45,  36},
    { -21, -17}, {  56,   2}, {   6,  30}, { -32,  38},
    {  39, -16}, {  83,   0}, {  67,  19}, {   0,  17},
    {  25, -13}, {  51,  -3}, {  33,   0}, {  11,   2},
    {  -1, -84}, {  85, -65}, { -21, -36}, { -24, -36},
};


void initializePSQT(){
    
    int sq, w32, b32;
    
    for (sq = 0; sq < SQUARE_NB; sq++){

        w32 = relativeSquare32(sq, WHITE);
        b32 = relativeSquare32(sq, BLACK);

        PSQTMidgame[WHITE_PAWN  ][sq] = + PieceValues[PAWN  ][MG] +   PawnPSQT32[w32][MG];
        PSQTEndgame[WHITE_PAWN  ][sq] = + PieceValues[PAWN  ][EG] +   PawnPSQT32[w32][EG];
        PSQTMidgame[WHITE_KNIGHT][sq] = + PieceValues[KNIGHT][MG] + KnightPSQT32[w32][MG];
        PSQTEndgame[WHITE_KNIGHT][sq] = + PieceValues[KNIGHT][EG] + KnightPSQT32[w32][EG];
        PSQTMidgame[WHITE_BISHOP][sq] = + PieceValues[BISHOP][MG] + BishopPSQT32[w32][MG];
        PSQTEndgame[WHITE_BISHOP][sq] = + PieceValues[BISHOP][EG] + BishopPSQT32[w32][EG];
        PSQTMidgame[WHITE_ROOK  ][sq] = + PieceValues[ROOK  ][MG] +   RookPSQT32[w32][MG];
        PSQTEndgame[WHITE_ROOK  ][sq] = + PieceValues[ROOK  ][EG] +   RookPSQT32[w32][EG];
        PSQTMidgame[WHITE_QUEEN ][sq] = + PieceValues[QUEEN ][MG] +  QueenPSQT32[w32][MG];
        PSQTEndgame[WHITE_QUEEN ][sq] = + PieceValues[QUEEN ][EG] +  QueenPSQT32[w32][EG];
        PSQTMidgame[WHITE_KING  ][sq] = + PieceValues[KING  ][MG] +   KingPSQT32[w32][MG];
        PSQTEndgame[WHITE_KING  ][sq] = + PieceValues[KING  ][EG] +   KingPSQT32[w32][EG];
        
        PSQTMidgame[BLACK_PAWN  ][sq] = - PieceValues[PAWN  ][MG] -   PawnPSQT32[b32][MG];
        PSQTEndgame[BLACK_PAWN  ][sq] = - PieceValues[PAWN  ][EG] -   PawnPSQT32[b32][EG];
        PSQTMidgame[BLACK_KNIGHT][sq] = - PieceValues[KNIGHT][MG] - KnightPSQT32[b32][MG];
        PSQTEndgame[BLACK_KNIGHT][sq] = - PieceValues[KNIGHT][EG] - KnightPSQT32[b32][EG];
        PSQTMidgame[BLACK_BISHOP][sq] = - PieceValues[BISHOP][MG] - BishopPSQT32[b32][MG];
        PSQTEndgame[BLACK_BISHOP][sq] = - PieceValues[BISHOP][EG] - BishopPSQT32[b32][EG];
        PSQTMidgame[BLACK_ROOK  ][sq] = - PieceValues[ROOK  ][MG] -   RookPSQT32[b32][MG];
        PSQTEndgame[BLACK_ROOK  ][sq] = - PieceValues[ROOK  ][EG] -   RookPSQT32[b32][EG];
        PSQTMidgame[BLACK_QUEEN ][sq] = - PieceValues[QUEEN ][MG] -  QueenPSQT32[b32][MG];
        PSQTEndgame[BLACK_QUEEN ][sq] = - PieceValues[QUEEN ][EG] -  QueenPSQT32[b32][EG];
        PSQTMidgame[BLACK_KING  ][sq] = - PieceValues[KING  ][MG] -   KingPSQT32[b32][MG];
        PSQTEndgame[BLACK_KING  ][sq] = - PieceValues[KING  ][EG] -   KingPSQT32[b32][EG];
    }
}
