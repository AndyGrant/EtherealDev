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
    { -23,   1}, {  12,   3}, {  -5,   6}, {  -5,   0},
    { -26,   0}, {  -3,   0}, {  -5,  -6}, {  -2, -11},
    { -21,   7}, {  -6,   6}, {   3, -10}, {   3, -22},
    { -12,  14}, {   2,   8}, {  -2,  -3}, {   4, -23},
    {   1,  25}, {  14,  24}, {  13,   4}, {  18, -22},
    { -49,   3}, { -46,   9}, {  -1, -17}, {   0, -35},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -36, -50}, {   3, -40}, { -11, -14}, {  11,  -8},
    {   9, -52}, {   5, -11}, {  15, -25}, {  20,  -3},
    {   4, -20}, {  27, -19}, {  15,   4}, {  30,  13},
    {   5,   8}, {  29,   9}, {  31,  33}, {  39,  37},
    {  27,   6}, {  37,  13}, {  39,  41}, {  49,  42},
    { -26,   9}, {  30,   6}, {  42,  38}, {  52,  35},
    { -44, -22}, { -43,   7}, {  47, -31}, {  13,  -4},
    {-178, -34}, {-116, -32}, {-164, -12}, { -66, -29},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {  19, -23}, {  23, -26}, {   2,  -9}, {  18, -15},
    {  35, -30}, {  32, -24}, {  24, -15}, {  10,  -2},
    {  25, -12}, {  31, -13}, {  23,   0}, {  19,   6},
    {  12,  -5}, {  13,  -1}, {  10,  14}, {  37,  19},
    { -10,  13}, {  25,   4}, {   8,  16}, {  36,  21},
    {  -5,   9}, {   1,   8}, {  30,   7}, {  23,   5},
    { -62,   4}, {   6,  -2}, {  -6, -13}, { -35,   1},
    { -43,   0}, { -59,  -3}, {-148,   5}, {-129,  13},
};

const int RookPSQT32[32][PHASE_NB] = {
    {  -5, -33}, {  -7, -17}, {   5, -14}, {  10, -20},
    { -34, -26}, {  -5, -27}, {   2, -20}, {  11, -26},
    { -19, -21}, {   5, -14}, {   1, -19}, {   2, -20},
    { -19,  -1}, { -11,   4}, {  -4,   3}, {   1,   2},
    { -11,  12}, { -10,   8}, {  21,   6}, {  23,   8},
    { -16,  15}, {  18,  10}, {  19,  15}, {  28,  14},
    {  -2,  16}, {  -9,  17}, {  42,   3}, {  24,  10},
    {  -1,  23}, {  15,  14}, { -20,  23}, {   9,  30},
};

const int QueenPSQT32[32][PHASE_NB] = {
    {  -2, -47}, { -11, -29}, {  -5, -20}, {  14, -40},
    {   5, -50}, {  13, -37}, {  19, -51}, {  15, -15},
    {   7, -24}, {  23, -18}, {   8,   5}, {   5,   3},
    {   6,  -5}, {  13,   4}, {   0,  14}, {  -1,  47},
    {  -8,   9}, { -11,  33}, {  -3,  22}, { -18,  53},
    {  -9,   3}, {   4,  21}, {   6,  21}, {  -5,  48},
    {   0,  14}, { -66,  56}, {  32,  13}, { -24,  70},
    { -30, -29}, {   5, -17}, {   8,  -9}, { -15,   9},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  82,-106}, {  90, -81}, {  38, -35}, {  21, -39},
    {  70, -54}, {  60, -46}, {  10,  -5}, { -23,   2},
    {  -2, -42}, {  45, -31}, {  17,   0}, { -14,  16},
    { -56, -36}, {  32, -20}, {   6,  15}, { -46,  37},
    { -23, -18}, {  56,   1}, {   4,  31}, { -34,  38},
    {  40, -17}, {  82,   0}, {  64,  18}, {  -1,  17},
    {  27, -13}, {  47,  -5}, {  33,  -1}, {   8,   1},
    {  -8, -92}, {  83, -67}, { -29, -40}, { -31, -39},
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
