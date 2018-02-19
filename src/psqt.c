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
    { -16,  -3}, {   6,  -2}, {  -8,   0}, {  -7,  -7},
    { -18,  -4}, {  -4,  -4}, { -10,  -9}, {  -8, -12},
    { -17,   0}, {  -5,   0}, {   0, -12}, {   0, -20},
    { -10,   5}, {   0,   3}, {  -1,  -5}, {   2, -19},
    {  -5,  12}, {   5,  13}, {   8,   0}, {   8, -15},
    { -32,   7}, { -29,   6}, {  -3, -11}, {   4, -21},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -42, -68}, { -23, -52}, { -23, -37}, { -12, -39},
    { -13, -56}, { -10, -40}, { -14, -49}, {  -8, -37},
    { -16, -41}, {  -5, -43}, { -11, -32}, {   0, -25},
    { -13, -31}, {  -6, -28}, {   0, -18}, {   1, -13},
    {  -3, -25}, {   0, -25}, {   5,  -8}, {   7,  -7},
    { -39, -26}, {  -5, -27}, {   8, -10}, {  12, -13},
    { -41, -45}, { -34, -29}, {  35, -57}, { -12, -42},
    {-110, -62}, { -78, -48}, {-103, -51}, { -77, -47},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {   2, -32}, {   0, -37}, { -19, -18}, {  -5, -28},
    {  13, -42}, {   5, -33}, {  -1, -30}, { -10, -23},
    {   2, -31}, {   4, -29}, {  -3, -20}, {  -4, -15},
    {  -6, -29}, { -10, -24}, {  -8, -13}, {   7, -12},
    { -28, -15}, {  -5, -18}, { -12, -13}, {   1,  -7},
    { -25, -17}, { -17, -17}, {  -2, -15}, {  -2, -16},
    { -70, -17}, {  -9, -23}, { -15, -32}, { -48, -27},
    { -34, -30}, { -53, -25}, {-100, -31}, {-112, -22},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -22, -30}, { -22, -24}, {  -8, -11}, {  -6, -23},
    { -36, -21}, { -17, -28}, { -16, -21}, {  -8, -27},
    { -25, -17}, { -10, -19}, { -20, -21}, { -12, -24},
    { -27, -11}, { -24, -10}, { -21, -14}, { -14, -10},
    { -23,  -2}, { -22,  -5}, {  -2,  -8}, {  -5,  -6},
    { -24,  -3}, { -14,  -3}, {  -3,  -5}, {   0,  -6},
    { -13,   0}, { -15,   0}, {  20, -12}, {   2,  -5},
    { -25,   5}, { -13,   0}, { -35,  -2}, { -10,   3},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -40, -66}, { -44, -63}, { -37, -42}, { -28, -67},
    { -24, -67}, { -23, -70}, { -23, -84}, { -27, -50},
    { -26, -55}, { -17, -53}, { -30, -44}, { -33, -38},
    { -30, -40}, { -26, -37}, { -36, -36}, { -36, -20},
    { -37, -25}, { -37, -22}, { -34, -29}, { -38, -12},
    { -34, -39}, { -21, -31}, { -15, -32}, { -26, -17},
    { -23, -36}, { -39, -17}, {   0, -42}, { -44, -14},
    { -40, -45}, { -26, -38}, { -22, -42}, { -43, -37},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  45, -66}, {  51, -55}, {  28, -20}, {  14, -26},
    {  44, -32}, {  37, -26}, {   7,  -2}, { -11,   1},
    {  -1, -23}, {  28, -15}, {  10,   0}, {  -4,  11},
    { -29, -23}, {  25,  -7}, {   3,   9}, { -25,  24},
    {  -7, -13}, {  45,   1}, {   9,  16}, { -11,  24},
    {  35,  -4}, {  56,   4}, {  41,  11}, {  19,  14},
    {   1,  -8}, {  39,   0}, {  33,  -2}, {  16,  -2},
    {  13, -40}, {  63, -35}, {  -2, -27}, {   3, -25},
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
