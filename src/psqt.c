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
    {  -4,  -5}, {   9,   0}, {  -1,   1}, {  -6,  -4},
    {  -9,  -6}, {  -1,  -2}, {  -7,  -4}, { -13, -11},
    {  -7,   0}, {  -9,   0}, {  -3,  -7}, {   1, -13},
    {  -3,   5}, {  -7,   3}, {  -3,  -3}, {   2, -10},
    {   3,   9}, {   3,   9}, {   3,  -3}, {   2, -14},
    {  -6,  -3}, {  -9,   1}, { -13, -19}, {   6, -21},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -27, -46}, { -20, -38}, { -17, -21}, { -10, -25},
    { -13, -32}, { -21, -26}, { -14, -26}, {  -5, -21},
    { -16, -24}, {  -8, -20}, {  -2, -12}, {   4,  -8},
    { -10, -12}, {  -8, -17}, {   0,  -8}, {   3,   1},
    {  -2,  -8}, {   3,  -8}, {   4,   2}, {  16,   4},
    { -14, -14}, {   2, -12}, {   2,   1}, {  13,   0},
    { -13, -28}, { -17, -20}, {  10, -31}, { -14, -19},
    { -80, -59}, { -49, -41}, { -77, -47}, { -41, -34},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {  -3, -19}, {   4, -18}, { -11, -10}, {  -4, -14},
    {  13, -20}, {   8, -17}, {   8, -15}, {  -8, -10},
    {  -1, -13}, {  14, -10}, {  -1, -11}, {   2,  -1},
    {  -1, -10}, { -14, -11}, {  -3,  -5}, {   4,  -2},
    { -21,  -5}, {  -5,  -4}, {  -2,  -3}, {   6,   0},
    {  -8,  -1}, {  -3,  -7}, {   1,  -6}, {  -1,  -8},
    { -33, -13}, { -13, -13}, { -14, -16}, { -34, -18},
    { -42, -28}, { -38, -20}, { -65, -32}, { -59, -18},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -24, -25}, { -20, -20}, {  -4,  -7}, {   0, -16},
    { -33, -20}, { -17, -26}, { -20, -22}, {  -6, -22},
    { -21, -18}, { -11, -17}, { -15, -20}, {  -9, -20},
    { -20,  -8}, { -17,  -8}, { -16, -14}, { -13, -11},
    { -13,  -1}, { -14,  -6}, {  -4,  -2}, {  -1,  -5},
    { -12,  -1}, {  -8,  -1}, {  -2,  -6}, {   1,  -3},
    { -11,  -2}, {  -6,  -1}, {   6, -12}, {  -8,  -5},
    { -18,   5}, {  -8,   0}, { -36, -12}, { -17,  -1},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -29, -38}, { -32, -43}, { -27, -26}, { -17, -48},
    { -18, -36}, { -17, -43}, { -20, -64}, { -14, -34},
    { -14, -27}, {  -6, -30}, { -15, -33}, { -17, -25},
    { -19, -26}, { -18, -29}, { -22, -31}, { -21, -19},
    { -26, -10}, { -23, -17}, { -18, -22}, { -21, -10},
    { -13, -15}, { -15, -22}, { -13, -20}, { -17,  -8},
    { -14, -18}, { -40, -18}, { -15, -33}, { -33, -11},
    { -53, -53}, { -40, -44}, { -63, -65}, { -52, -44},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  -7, -28}, {   7, -19}, {   1,   0}, { -20,  -9},
    {   1,  -8}, {  10,   0}, {  -2,  10}, { -18,  10},
    { -16,  -6}, {   6,   2}, {   8,  13}, {   2,  16},
    { -25,  -6}, {   6,   4}, {  15,  18}, {   3,  19},
    { -11,   3}, {  23,  13}, {  21,  20}, {  13,  18},
    {  12,  13}, {  41,  21}, {  35,  19}, {  18,  12},
    {  13,  12}, {  22,  18}, {  20,   7}, {  26,   7},
    {  -3,  -3}, {  21,   8}, {   6, -12}, {   4,  -3},
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
