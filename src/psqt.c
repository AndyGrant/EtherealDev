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
    { -23,   1}, {  12,   3}, {  -5,   5}, {  -6,  -1},
    { -25,   0}, {  -3,  -1}, {  -5,  -6}, {  -2, -10},
    { -21,   7}, {  -6,   6}, {   4,  -9}, {   3, -22},
    { -10,  14}, {   3,   8}, {   0,  -2}, {   4, -22},
    {   3,  26}, {  15,  24}, {  18,   4}, {  18, -21},
    { -43,   7}, { -39,  10}, {  -1, -16}, {   4, -32},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -36, -46}, {   3, -39}, {  -9, -12}, {  10,  -7},
    {   9, -49}, {   6, -10}, {  16, -24}, {  21,  -2},
    {   5, -18}, {  28, -17}, {  16,   4}, {  30,  14},
    {   6,   9}, {  26,   9}, {  30,  33}, {  37,  36},
    {  27,   8}, {  33,  14}, {  37,  40}, {  45,  41},
    { -25,  10}, {  34,   7}, {  40,  38}, {  50,  35},
    { -41, -19}, { -38,   7}, {  54, -28}, {  14,  -2},
    {-163, -30}, {-109, -30}, {-157,  -7}, { -60, -24},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {  18, -21}, {  20, -23}, {   1,  -9}, {  18, -14},
    {  33, -28}, {  33, -23}, {  23, -14}, {  11,  -2},
    {  24, -11}, {  31, -12}, {  24,   0}, {  18,   6},
    {  11,  -4}, {  12,   0}, {   9,  14}, {  34,  19},
    { -11,  13}, {  22,   4}, {   7,  16}, {  32,  20},
    {  -6,   9}, {   3,   8}, {  31,   7}, {  21,   5},
    { -62,   5}, {  11,  -3}, {  -3, -11}, { -34,   2},
    { -33,   0}, { -58,  -3}, {-141,   7}, {-125,  14},
};

const int RookPSQT32[32][PHASE_NB] = {
    {  -3, -31}, {  -6, -16}, {   5, -13}, {  11, -20},
    { -33, -24}, {  -6, -26}, {   2, -19}, {  10, -25},
    { -19, -20}, {   3, -13}, {   0, -19}, {   1, -19},
    { -20,   0}, { -13,   4}, {  -6,   2}, {   0,   2},
    { -12,  13}, {  -8,   9}, {  18,   7}, {  20,   7},
    { -14,  16}, {  16,  11}, {  17,  14}, {  23,  13},
    {   0,  17}, {  -6,  17}, {  39,   3}, {  23,   9},
    {  -1,  23}, {  13,  14}, { -19,  23}, {   6,  28},
};

const int QueenPSQT32[32][PHASE_NB] = {
    {  -2, -45}, { -12, -27}, {  -4, -17}, {  17, -38},
    {   5, -48}, {  13, -36}, {  19, -49}, {  15, -15},
    {   5, -22}, {  22, -18}, {   5,   4}, {   3,   3},
    {   5,  -5}, {   5,   3}, {  -4,  11}, {  -7,  45},
    { -15,   8}, { -15,  31}, { -10,  20}, { -25,  50},
    { -13,   1}, {   0,  20}, {   1,  18}, { -12,  45},
    {   3,  13}, { -57,  54}, {  28,  12}, { -21,  68},
    { -24, -31}, {   2, -17}, {   2, -11}, { -16,   8},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  78,-103}, {  87, -79}, {  37, -34}, {  20, -37},
    {  69, -53}, {  61, -45}, {  13,  -5}, { -17,   1},
    {   0, -40}, {  46, -29}, {  19,   0}, { -10,  14},
    { -50, -33}, {  32, -19}, {   7,  15}, { -44,  35},
    { -22, -17}, {  56,   2}, {   7,  30}, { -32,  37},
    {  39, -16}, {  83,   1}, {  66,  19}, {   1,  17},
    {  24, -14}, {  56,  -2}, {  37,   2}, {  17,   4},
    {   5, -80}, {  90, -61}, { -17, -35}, { -23, -33},
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
