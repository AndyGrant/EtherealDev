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
    { -15,  -2}, {   5,   0}, {  -5,   0}, {  -7, -12},
    { -17,  -2}, {  -1,  -1}, {  -8,  -4}, {  -7, -12},
    { -15,   0}, {  -3,   1}, {   0,  -7}, {   1, -19},
    {  -7,   2}, {   1,   3}, {   4,  -6}, {   6, -15},
    {  10,  -2}, {  20,   2}, {  29,  -2}, {  19, -15},
    {  10, -64}, {  19, -20}, {  38, -16}, {  40, -22},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -48,-130}, { -25, -63}, { -39, -61}, { -22, -47},
    { -22, -56}, { -31, -50}, { -21, -51}, {  -7, -42},
    { -26, -50}, { -11, -43}, { -14, -34}, {   2, -29},
    { -15, -30}, {  -9, -28}, {  -4, -20}, {   0, -14},
    {  -5, -20}, {   0, -19}, {   8,  -9}, {  10, -11},
    { -16, -10}, {   0, -21}, {   8,  -9}, {  25, -11},
    { -32, -31}, { -27, -29}, {  18, -41}, {  -8, -40},
    { -96,-107}, { -42, -28}, { -87, -62}, { -41, -35},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {  -4, -35}, { -14, -32}, { -21, -23}, { -16, -34},
    {  10, -44}, {   2, -32}, {  -1, -30}, { -13, -23},
    {  -4, -30}, {   9, -32}, {  -5, -24}, {  -2, -24},
    {   0, -18}, { -13, -16}, {  -9, -14}, {   4, -17},
    { -32, -13}, { -10,  -8}, {  -6, -16}, {   7, -17},
    { -14,  -9}, {  -4, -17}, {   4, -11}, {   0, -19},
    { -45, -14}, { -20, -18}, { -14, -21}, { -32, -12},
    { -41, -26}, { -37, -27}, { -65, -24}, { -78, -29},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -22, -31}, { -25, -28}, { -10, -15}, {  -7, -26},
    { -45, -37}, { -25, -34}, { -26, -29}, { -15, -33},
    { -30, -20}, { -19, -18}, { -20, -22}, { -20, -22},
    { -23,  -5}, { -11,   1}, { -15,  -6}, { -17,  -5},
    {  -9,   6}, { -11,   0}, {   7,   0}, {   7,   0},
    { -10,   0}, {  -2,  -6}, {   7,  -4}, {   9,   0},
    {  -8,   0}, { -13,  -7}, {  12, -12}, {   6,   1},
    {  -1,  21}, {   1,   6}, { -21,   6}, {  -3,  12},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -35, -65}, { -43, -64}, { -44, -44}, { -24, -66},
    { -30, -58}, { -24, -64}, { -20, -83}, { -25, -46},
    { -24, -48}, { -13, -41}, { -26, -36}, { -31, -30},
    { -29, -30}, { -24, -33}, { -32, -28}, { -32, -10},
    { -32, -11}, { -34, -14}, { -22, -12}, { -33,  -4},
    { -21, -20}, { -21, -22}, {  -9, -19}, { -26,  -6},
    { -22, -18}, { -61, -13}, { -10, -31}, { -38,  -8},
    { -33, -50}, { -29, -43}, { -44, -51}, { -43, -40},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  44, -66}, {  59, -55}, {  33, -21}, {  10, -34},
    {  41, -27}, {  37, -19}, {  13,   2}, {  -7,   2},
    {   0, -13}, {  25, -10}, {  14,   5}, {  -5,  12},
    {  -7, -25}, {  21,  -4}, {   0,   4}, { -37,  16},
    {   6, -13}, {  37,  -7}, {  -9,   0}, { -50,   1},
    {  43,  -5}, {  54,  -8}, {  18, -10}, { -16,  -8},
    {   3, -39}, {  54, -14}, {  51,  -6}, {  53,  -8},
    { -85,-103}, {  81, -11}, {  38,  -6}, {  71,  15},
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
