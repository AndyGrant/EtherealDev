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
    { -23,   0}, { -15,   0}, {   0,   0}, {   7,   0},
    { -28,  -4}, {   6,  -3}, {  -9,   0}, { -12, -10},
    { -28,  -6}, {  -7,  -7}, { -10, -12}, { -10, -15},
    { -26,   0}, { -10,   0}, {  -3, -14}, {  -1, -26},
    { -18,   7}, {  -1,   1}, {  -4,  -9}, {  -1, -28},
    {  -1,  17}, {  10,  15}, {  21,  -3}, {  10, -29},
    { -26,   3}, { -10,   4}, {   9, -21}, {   9, -35},
    { -23,   0}, { -15,   0}, {   0,   0}, {   7,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -56, -93}, { -28, -92}, { -50, -64}, { -31, -62},
    { -21,-100}, { -42, -67}, { -15, -76}, { -15, -56},
    { -29, -68}, {  -3, -70}, { -15, -46}, {  -7, -37},
    { -25, -39}, {  -9, -45}, {  -4, -18}, {   3, -17},
    {  -4, -42}, {   4, -37}, {  -6, -14}, {  14, -12},
    { -54, -45}, {  14, -45}, {  10, -14}, {  14, -17},
    { -78, -67}, { -73, -46}, {  37, -73}, { -17, -50},
    {-175, -76}, {-118, -79}, {-171, -54}, { -76, -70},
};

const int BishopPSQT32[32][PHASE_NB] = {
    { -25, -53}, {  -9, -48}, { -25, -45}, {  -9, -46},
    {  -4, -59}, {   4, -54}, {  -6, -46}, { -18, -34},
    {  -4, -43}, {   4, -43}, {  -1, -31}, {  -6, -25},
    { -21, -34}, { -14, -31}, { -14, -17}, {   0, -12},
    { -32, -18}, {  -7, -29}, { -15, -15}, {   7, -12},
    { -35, -23}, {  -9, -23}, {   7, -23}, {  -7, -28},
    { -70, -25}, {  -9, -37}, { -25, -40}, { -62, -28},
    { -57, -35}, { -78, -35}, {-164, -26}, {-140, -15},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -26, -46}, { -29, -29}, { -17, -26}, { -10, -34},
    { -53, -39}, { -32, -39}, { -28, -35}, { -12, -40},
    { -53, -37}, { -26, -28}, { -21, -37}, { -23, -34},
    { -51, -17}, { -34,  -9}, { -31, -12}, { -26, -14},
    { -43,   0}, { -29,  -7}, {  -3,  -6}, {  -3,  -7},
    { -34,   3}, {  -4,  -1}, {   0,   0}, {  -7,  -1},
    { -18,   4}, { -21,   3}, {  15, -12}, {  -3,  -7},
    { -21,  12}, {   0,   1}, { -37,   9}, {  -4,  15},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -53,-104}, { -64, -84}, { -57, -76}, { -35, -98},
    { -51,-110}, { -40, -96}, { -29,-109}, { -37, -78},
    { -43, -82}, { -25, -81}, { -46, -62}, { -43, -59},
    { -48, -68}, { -50, -62}, { -48, -54}, { -56, -21},
    { -56, -53}, { -60, -35}, { -53, -46}, { -68, -18},
    { -40, -59}, { -43, -42}, { -39, -48}, { -51, -21},
    { -29, -46}, { -85, -10}, { -15, -51}, { -46,   7},
    { -53, -96}, { -46, -82}, { -59, -82}, { -50, -51},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  70, -96}, {  82, -75}, {  34, -34}, {  17, -34},
    {  65, -50}, {  62, -42}, {  21,  -6}, {  -6,   0},
    {  -1, -39}, {  46, -26}, {  23,  -1}, {  -4,  12},
    { -50, -35}, {  21, -23}, {   9,  14}, { -45,  31},
    { -37, -21}, {  46,   0}, {   9,  28}, { -34,  34},
    {  34, -17}, {  78,   4}, {  57,  18}, {  -4,  17},
    {  20, -17}, {  68,   0}, {  48,  10}, {  46,  15},
    {   1, -71}, { 110, -48}, {   6, -28}, {  -7, -21},
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
