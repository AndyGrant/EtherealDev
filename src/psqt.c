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
    { -11,  -6}, {   5,   1}, {  -7,   0}, { -11,  -6},
    {  -7,  -7}, {   3,  -1}, {  -3,  -4}, {  -8,  -3},
    {  -7,   0}, {   1,   2}, {   5,  -5}, {   2, -10},
    {   2,   7}, {  10,   9}, {   5,   1}, {  11,  -1},
    {  32,  20}, {  42,  28}, {  57,  24}, {  48,  19},
    { 138,   9}, { 121,  41}, { 140,  53}, { 134,  47},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -55, -78}, { -18, -43}, { -31, -42}, { -17, -28},
    { -20, -35}, { -21, -20}, { -10, -26}, {   1, -17},
    { -20, -32}, {   2, -13}, {  -1, -11}, {  14,   0},
    {  -3, -13}, {  11,   0}, {  10,   7}, {  15,   8},
    {  14,  -4}, {  13,   8}, {  30,  14}, {  28,  16},
    {   0,  -1}, {  27,   0}, {  39,  11}, {  61,  13},
    { -14,  -9}, {   0,  -1}, {  48,   0}, {  42,   0},
    {-102, -60}, { -35,  -2}, { -49,  -5}, {  -2,   6},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {   7, -33}, {   4, -26}, { -18, -18}, { -12, -22},
    {  13, -29}, {  12, -10}, {  11, -10}, {  -3,  -5},
    {   2, -11}, {  19,  -5}, {   7,   2}, {   9,   3},
    {  13,  -4}, {   5,   2}, {   6,   9}, {  27,   5},
    { -13,  -6}, {   7,   9}, {  20,   4}, {  40,   7},
    {   6,  -4}, {  20,   0}, {  38,   8}, {  31,   5},
    { -18, -10}, {   8,   0}, {   4,   0}, {  11,   6},
    { -24, -15}, { -32,  -7}, { -27,  -6}, { -21,  -3},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -11, -12}, { -13,  -7}, {   0, -10}, {   1, -10},
    { -42, -15}, { -23, -13}, { -11, -17}, { -13, -16},
    { -26,  -7}, {  -7,  -5}, {  -8,  -7}, { -17,  -6},
    {  -5,   5}, {   5,  10}, {   3,   9}, {   0,   3},
    {  21,  13}, {  13,  15}, {  37,  15}, {  36,  16},
    {  29,  13}, {  42,  11}, {  47,  15}, {  52,  17},
    {  27,  17}, {  11,  13}, {  34,  13}, {  44,  20},
    {  46,  24}, {  45,  19}, {  34,  13}, {  47,  22},
};

const int QueenPSQT32[32][PHASE_NB] = {
    {  -7, -22}, { -17, -32}, { -22, -27}, {  -3, -27},
    { -17, -31}, {  -8, -22}, {   5, -21}, {  -2, -13},
    {   0, -10}, {   7,  -2}, {   2,   3}, {  -5,   0},
    {  -1,   7}, {  10,   8}, {   3,  16}, {   0,  25},
    {  14,  15}, {   4,  28}, {  17,  36}, {   7,  36},
    {  12,  17}, {  34,  23}, {  34,  34}, {  38,  34},
    {   0,  15}, { -19,  21}, {  29,  22}, {  27,  30},
    {  13,   7}, {  17,   8}, {  29,  13}, {  28,  15},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  -9, -22}, {  20, -10}, { -13, -13}, { -17, -22},
    {  -1,  -1}, {  -8,   1}, { -18,   1}, { -26,   2},
    { -26,   1}, {   1,   6}, {   7,   7}, {   3,  10},
    {  -2,  -9}, {  33,  13}, {  26,  13}, {   1,  11},
    {  15,  -6}, {  73,  22}, {  37,  14}, { -18,  -2},
    {  29,   0}, {  87,  16}, {  75,  12}, {  13, -11},
    {   6, -14}, { 101,  17}, { 107,  14}, { 104,   2},
    { -62, -70}, {  78,  10}, {  87,  17}, {  76, -20},
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
