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
    { -14,  -2}, {   5,   0}, {  -2,   0}, {  -6,  -6},
    { -17,  -4}, {  -5,  -4}, {  -8,  -6}, { -11, -14},
    { -16,   1}, { -13,   0}, {  -3,  -9}, {   3, -17},
    { -11,   7}, { -10,   3}, {  -2,  -5}, {   7, -16},
    {  -3,  12}, {   1,   9}, {   7,  -5}, {   6, -18},
    { -12,   5}, { -14,   8}, { -10, -15}, {  11, -16},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -40, -38}, { -28, -30}, { -27, -20}, { -19, -23},
    { -24, -27}, { -28, -25}, { -22, -26}, { -13, -20},
    { -23, -22}, { -14, -20}, { -10, -14}, {  -4, -12},
    { -16, -15}, { -17, -19}, {  -8, -11}, {  -2,  -5},
    {  -9, -12}, {  -1, -12}, {  -5,  -5}, {   6,  -1},
    { -24, -16}, {  -8, -14}, {  -7,  -4}, {   2,  -5},
    { -26, -27}, { -26, -21}, {  -2, -27}, { -24, -19},
    { -78, -51}, { -58, -38}, { -81, -43}, { -50, -32},
};

const int BishopPSQT32[32][PHASE_NB] = {
    { -12, -25}, {   0, -24}, { -16, -19}, { -12, -21},
    {  10, -30}, {   6, -28}, {   6, -24}, { -14, -18},
    {  -4, -22}, {  10, -18}, {  -6, -20}, {  -1,  -8},
    {  -4, -20}, { -22, -19}, {  -9, -13}, {   0, -12},
    { -28, -13}, { -11, -13}, {  -7, -13}, {   0,  -7},
    { -14, -11}, {  -8, -16}, {  -4, -15}, {  -5, -16},
    { -44, -19}, { -21, -20}, { -21, -24}, { -45, -24},
    { -49, -38}, { -51, -27}, { -79, -42}, { -75, -23},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -25, -29}, { -23, -23}, { -10,  -6}, {  -2, -18},
    { -38, -21}, { -21, -27}, { -26, -23}, {  -9, -24},
    { -25, -19}, { -15, -19}, { -21, -21}, { -13, -22},
    { -26,  -7}, { -21,  -7}, { -22, -14}, { -19, -10},
    { -19,   0}, { -19,  -5}, {  -6,  -2}, {  -5,  -5},
    { -17,   0}, { -13,   0}, {  -5,  -5}, {   0,  -3},
    { -16,  -3}, { -11,  -2}, {   5, -15}, { -11,  -8},
    { -23,   8}, { -11,   2}, { -43,  -9}, { -19,   1},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -38, -56}, { -42, -55}, { -39, -36}, { -27, -63},
    { -26, -51}, { -25, -58}, { -28, -86}, { -24, -44},
    { -22, -40}, { -15, -39}, { -27, -40}, { -29, -29},
    { -29, -34}, { -28, -35}, { -34, -34}, { -34, -16},
    { -34, -17}, { -33, -18}, { -27, -24}, { -32,  -9},
    { -16, -26}, { -23, -28}, { -17, -23}, { -26,  -8},
    { -18, -27}, { -55, -17}, { -18, -38}, { -42, -11},
    { -53, -61}, { -40, -49}, { -66, -71}, { -51, -45},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  45, -71}, {  56, -58}, {  35, -23}, {  15, -32},
    {  42, -34}, {  47, -23}, {  15,  -4}, {  -7,   0},
    {   4, -24}, {  23, -13}, {   7,   3}, { -11,  10},
    { -22, -20}, {   9,  -7}, {   2,  10}, { -30,  16},
    { -13,  -8}, {  19,   3}, {   0,  14}, { -24,  15},
    {  14,   1}, {  39,  12}, {  15,  11}, { -19,   6},
    {  14,   0}, {  20,   9}, {  -2,   1}, { -12,   2},
    {   6, -20}, {  22,  -2}, { -14, -23}, { -31,  -9},
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
