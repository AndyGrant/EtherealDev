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
    { -17,  -2}, {   5,  -3}, {  -7,  -1}, {  -8, -10},
    { -18,  -3}, {  -4,  -3}, { -10,  -6}, {  -6, -11},
    { -14,   0}, {  -3,   0}, {   1,  -8}, {   0, -18},
    {  -6,   0}, {   6,   0}, {  -1,  -6}, {   0, -12},
    {   9,   1}, {  19,   5}, {  25,   1}, {   8,  -6},
    {  46, -60}, {  15, -23}, {  26, -18}, {  32, -28},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -31,-112}, { -15, -73}, { -28, -67}, { -14, -54},
    { -14, -70}, { -19, -59}, {  -9, -68}, {  -4, -54},
    { -16, -62}, {   0, -59}, {  -9, -44}, {   3, -40},
    { -13, -34}, {  -1, -34}, {   1, -22}, {   7, -21},
    {  -4, -24}, {   6, -28}, {   7, -11}, {  15, -13},
    { -35, -13}, {   4, -26}, {   9,  -7}, {  20,  -8},
    { -53, -38}, { -44, -33}, {  28, -51}, {   6, -47},
    {-110, -38}, { -73, -32}, {-111, -30}, { -53, -33},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {   5, -48}, {  -5, -41}, { -17, -33}, {  -5, -45},
    {   5, -51}, {   4, -43}, {   0, -37}, { -10, -32},
    {  -2, -39}, {   7, -39}, {  -2, -30}, {  -2, -30},
    {  -9, -22}, {  -8, -25}, {  -9, -18}, {   5, -21},
    { -33, -14}, {  -4, -19}, { -10, -16}, {   8, -16},
    { -26, -12}, {  -9, -16}, {   4, -16}, {  -5, -21},
    { -54, -16}, {  -8, -26}, { -18, -28}, { -34, -23},
    { -31, -24}, { -44, -29}, {-102, -21}, { -84, -21},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -14, -38}, { -17, -28}, {  -8, -27}, {  -7, -31},
    { -36, -37}, { -19, -33}, { -15, -32}, {  -8, -36},
    { -29, -26}, { -14, -21}, { -14, -25}, { -21, -21},
    { -28,  -9}, { -14,  -5}, { -18,  -5}, { -17,  -8},
    { -17,   0}, { -12,  -3}, {   8,  -4}, {   6,  -2},
    { -13,   0}, {  11,  -8}, {  16,  -7}, {  10,  -3},
    {  -9,  -1}, { -16,  -4}, {   8, -10}, {   0,  -3},
    {   0,  12}, {  12,   0}, { -13,   3}, {   7,  10},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -25, -84}, { -38, -72}, { -37, -63}, { -22, -72},
    { -34, -80}, { -21, -78}, { -16, -81}, { -22, -57},
    { -26, -62}, { -15, -57}, { -30, -38}, { -29, -41},
    { -28, -48}, { -25, -46}, { -31, -30}, { -36, -10},
    { -36, -32}, { -37, -20}, { -33, -22}, { -44,  -4},
    { -28, -30}, { -22, -30}, { -19, -29}, { -25, -16},
    { -20, -26}, { -60,  -5}, {  -2, -39}, { -19, -11},
    { -11, -76}, { -11, -67}, { -18, -64}, {  -7, -60},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  46, -61}, {  56, -45}, {  19, -17}, {   9, -22},
    {  37, -25}, {  27, -21}, {   7,   1}, { -15,   4},
    { -20, -14}, {  22, -16}, {  14,   2}, {  -5,  10},
    { -13, -29}, {  31, -13}, {  23,   7}, { -37,  19},
    {  20, -23}, { 103, -15}, {  24,   3}, { -55,   7},
    { 108, -27}, { 141, -23}, {  99, -15}, {  35, -10},
    {  96, -53}, { 156, -36}, { 125, -13}, { 130, -10},
    { -21,-110}, { 158, -33}, {  59,  -8}, { 140, -16},
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
