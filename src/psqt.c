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
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
    { -16,  -2}, {   3,  -1}, {  -6,   0}, {  -6,  -3},
    { -19,  -3}, {  -6,  -4}, {  -7,  -8}, {  -4, -10},
    { -16,   0}, {  -7,   0}, {  -1,  -9}, {   0, -18},
    { -12,   4}, {  -2,   0}, {  -5,  -6}, {  -1, -19},
    {  -3,  12}, {   8,  13}, {   9,   1}, {   8, -16},
    { -17,  -2}, { -10,   3}, {   3, -11}, {   4, -24},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0}
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -38, -68}, { -16, -60}, { -33, -40}, { -16, -43},
    { -16, -63}, { -20, -49}, {  -9, -53}, { -10, -37},
    { -18, -44}, {  -3, -46}, { -11, -30}, {  -2, -26},
    { -16, -31}, {  -4, -33}, {  -2, -14}, {   2, -12},
    {   0, -27}, {   3, -28}, {   0,  -7}, {  10,  -7},
    { -33, -28}, {   1, -29}, {  13,  -8}, {   9, -10},
    { -56, -45}, { -41, -34}, {  26, -53}, {  -2, -38},
    {-124, -57}, { -95, -45}, {-122, -35}, { -55, -49}
};

const int BishopPSQT32[32][PHASE_NB] = {
    {  -7, -35}, {   0, -41}, { -14, -28}, {  -7, -32},
    {   3, -44}, {   2, -37}, {  -3, -33}, { -11, -22},
    {  -2, -31}, {   2, -32}, {  -3, -21}, {  -5, -19},
    { -15, -27}, {  -8, -22}, { -10, -13}, {   0,  -9},
    { -23, -14}, {  -4, -20}, {  -9, -13}, {   3,  -8},
    { -26, -11}, {  -8, -16}, {   5, -16}, {  -8, -18},
    { -51, -16}, {  -7, -23}, { -13, -32}, { -43, -21},
    { -36, -17}, { -47, -28}, {-117, -20}, { -78, -18}
};

const int RookPSQT32[32][PHASE_NB] = {
    { -17, -29}, { -18, -22}, { -11, -18}, {  -8, -22},
    { -34, -29}, { -17, -27}, { -15, -24}, {  -6, -27},
    { -28, -25}, { -13, -20}, { -13, -25}, { -13, -23},
    { -29, -11}, { -16,  -8}, { -19,  -8}, { -15,  -8},
    { -22,  -2}, { -16,  -5}, {   0,  -3}, {   1,  -4},
    { -20,   1}, {   3,  -3}, {   3,   0}, {  -3,   0},
    { -11,   1}, { -11,   0}, {  11,  -9}, {   0,  -4},
    {   1,   8}, {  11,   1}, {  -8,   6}, {  14,  11}
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -32, -74}, { -39, -62}, { -33, -56}, { -23, -62},
    { -26, -78}, { -24, -65}, { -19, -73}, { -22, -51},
    { -24, -56}, { -16, -51}, { -30, -38}, { -28, -37},
    { -32, -42}, { -29, -37}, { -31, -31}, { -36, -11},
    { -35, -35}, { -36, -21}, { -34, -25}, { -44,  -9},
    { -28, -35}, { -22, -31}, { -23, -28}, { -29, -11},
    { -11, -33}, { -50,   2}, {  -2, -28}, { -25,  11},
    { -16, -53}, { -10, -47}, {  -8, -38}, {  -6, -30}
};

const int KingPSQT32[32][PHASE_NB] = {
    {  47, -65}, {  54, -47}, {  22, -21}, {  10, -23},
    {  42, -30}, {  37, -26}, {  12,  -2}, {  -7,   0},
    { -12, -26}, {  29, -18}, {  13,   0}, {  -5,  10},
    { -64, -27}, {  12, -13}, {  10,   9}, { -28,  22},
    { -39, -17}, {  30,   0}, {   3,  16}, { -22,  21},
    {  40, -11}, {  46,   0}, {  26,  10}, {  -9,   9},
    {  23, -20}, {  31,  -4}, {  17,   2}, {   3,   7},
    {  45, -62}, {  98, -32}, { -16, -14}, { -17, -14}
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
