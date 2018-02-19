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
    { -23,  -4}, {   5,  -3}, {  -5,  -2}, { -14, -13},
    { -24,  -4}, {  -3,  -2}, { -12,  -8}, { -19, -18},
    { -15,  -1}, {   5,   0}, {   1,  -9}, {   0, -21},
    { -10,   2}, {  14,   4}, {   0,  -2}, {   8, -16},
    {   3,  15}, {  23,  13}, {  25,   7}, {  19,  -9},
    { -47,   1}, { -15,  -1}, {   0,  -9}, {   9, -24},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -26, -58}, { -34, -54}, { -38, -44}, { -15, -40},
    { -18, -63}, { -17, -33}, { -15, -50}, { -17, -36},
    { -20, -48}, { -11, -41}, { -16, -37}, {   0, -26},
    { -12, -34}, {   0, -22}, {  -5, -20}, {  -3, -12},
    {  -5, -18}, {  -1, -24}, {   2, -14}, {   5,  -3},
    { -28, -24}, {  -2, -22}, {   3, -11}, {  12, -13},
    { -30, -32}, { -23, -29}, {  31, -49}, {  -8, -31},
    { -95, -48}, { -54, -46}, { -92, -17}, { -29, -49},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {   5, -35}, {   4, -40}, { -26, -21}, { -10, -34},
    {  14, -53}, {   0, -34}, {   0, -29}, { -13, -25},
    {  -2, -32}, {   7, -36}, {  -5, -21}, {  -5, -19},
    {  -2, -31}, { -11, -16}, {  -9, -13}, {   1, -15},
    { -35, -11}, {  -8, -17}, { -10, -19}, {   3,  -6},
    { -17, -15}, { -20, -15}, {   0, -14}, {  -5, -11},
    { -43, -12}, {   5, -20}, {  -7, -32}, { -42, -12},
    { -23, -19}, { -48, -27}, { -78, -11}, { -77, -23},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -24, -30}, { -17, -28}, { -16, -19}, {  -9, -27},
    { -19, -36}, { -17, -32}, { -21, -26}, { -11, -30},
    { -24, -23}, {  -5, -27}, { -22, -22}, { -16, -24},
    { -20, -18}, { -17,  -9}, { -20, -17}, { -20, -12},
    { -18,  -1}, { -13,  -8}, {  -1,  -3}, {   0,  -3},
    { -19,  -2}, {   2,  -2}, {  -5,  -2}, {   3,  -1},
    { -12,   0}, { -17,   0}, {  10, -11}, {   1,  -4},
    { -14,   2}, {  -9,   2}, { -21,   0}, { -21,   3},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -43, -69}, { -33, -68}, { -36, -56}, { -31, -65},
    { -27, -68}, { -19, -86}, { -21, -81}, { -27, -49},
    { -26, -60}, { -16, -61}, { -28, -43}, { -31, -38},
    { -25, -39}, { -26, -40}, { -29, -39}, { -37, -10},
    { -33, -20}, { -33, -22}, { -31, -21}, { -38,  -5},
    { -24, -25}, { -16, -30}, { -11, -23}, { -13, -24},
    { -30, -22}, { -36, -23}, {  -4, -36}, { -40, -13},
    { -28, -41}, { -33, -35}, { -25, -50}, { -48, -35},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  53, -72}, {  53, -54}, {  24, -25}, {   9, -29},
    {  34, -34}, {  35, -32}, {  10,  -3}, {  -9,  -2},
    {  -1, -22}, {  24, -14}, {   6,   1}, { -10,  12},
    { -12, -14}, {  35,  -6}, {  18,  12}, { -21,  26},
    {  35,  -6}, {  41,  11}, {  23,  22}, { -10,  29},
    {  72,   5}, {  51,   6}, {  60,  17}, {  50,  30},
    {  50,   0}, {  17,  10}, {  19,  13}, { 113,   7},
    { -33, -36}, {  84, -39}, {  29, -17}, {  26,  16},
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
