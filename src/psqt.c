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
    { -11,  -9}, {   4,  -2}, {  -7,  -2}, {  -7,  -8},
    {  -8, -10}, {   1,  -4}, {  -3,  -7}, {  -7,  -6},
    {  -9,  -4}, {  -2,  -1}, {   5,  -8}, {   0, -13},
    {  -1,   0}, {   4,   2}, {  -1,  -4}, {   2,  -6},
    {  18,   7}, {  24,  15}, {  32,  11}, {  24,   5},
    { 106, -10}, {  62,  23}, { 101,  34}, {  88,  29},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -34, -76}, { -15, -43}, { -22, -43}, {  -9, -28},
    { -14, -36}, { -15, -24}, { -10, -33}, {  -1, -24},
    { -15, -35}, {   0, -23}, {  -6, -18}, {   8, -12},
    {  -4, -17}, {   3,  -9}, {   3,  -3}, {   6,  -2},
    {   6,  -8}, {   5,  -2}, {  14,   2}, {  12,   2},
    { -10,  -5}, {  10,  -8}, {  16,   1}, {  35,   1},
    { -12, -18}, { -11, -12}, {  28, -16}, {  18, -15},
    {-130, -43}, { -46, -12}, { -66, -11}, { -21,  -4},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {  10, -36}, {   2, -29}, { -17, -17}, {  -4, -23},
    {  10, -33}, {   8, -15}, {   6, -15}, {  -4,  -9},
    {   1, -17}, {  13, -12}, {   3,  -2}, {   2,  -2},
    {   6, -11}, {  -1,  -4}, {   0,   2}, {  12,   0},
    { -19, -10}, {   1,   1}, {   3,  -1}, {  18,   2},
    {  -6,  -7}, {   6,  -4}, {  14,   2}, {  11,  -1},
    { -24, -15}, {  -7,  -7}, {  -7,  -6}, { -12,   0},
    { -41, -21}, { -43, -15}, { -49, -12}, { -41,  -9},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -11, -16}, { -13, -10}, {  -2, -14}, {  -1, -12},
    { -34, -15}, { -20, -15}, { -10, -19}, {  -8, -17},
    { -24,  -9}, { -10, -10}, {  -9, -11}, { -13, -10},
    { -11,   0}, {  -3,   3}, {  -4,   2}, {  -3,  -3},
    {   7,   6}, {   3,   6}, {  21,   5}, {  19,   4},
    {  12,   4}, {  22,   0}, {  22,   3}, {  26,   5},
    {   3,   7}, {  -9,   3}, {   9,   1}, {  17,   6},
    {  26,  19}, {  27,  13}, {  16,   6}, {  26,  16},
};

const int QueenPSQT32[32][PHASE_NB] = {
    {  -5, -24}, { -13, -29}, { -14, -22}, {   0, -20},
    {  -9, -26}, {  -2, -20}, {   5, -19}, {   1,  -7},
    {   0, -14}, {   7,  -5}, {   0,   2}, {  -3,   0},
    {  -1,  -2}, {   0,   1}, {  -4,   7}, {  -4,  18},
    {  -3,   7}, {  -5,  16}, {  -4,  21}, {  -6,  23},
    {  -4,  12}, {  11,  12}, {   9,  18}, {  12,  18},
    {  -6,  11}, { -25,  17}, {  11,  11}, {   1,  14},
    { -12,  -3}, {  -4,  -7}, {   2,  -4}, {  -4,  -6},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  -6, -20}, {  10, -11}, {  -8,  -8}, { -12, -14},
    {   1,   1}, {  -6,   1}, { -10,   4}, { -14,   5},
    { -23,   5}, {   2,   6}, {  11,   7}, {   7,  10},
    {   0,  -6}, {  31,  11}, {  24,  11}, {  -3,   9},
    {  17,  -7}, {  67,  13}, {  32,   8}, { -25,  -7},
    {  26,   0}, {  71,   3}, {  71,   2}, {   7, -18},
    {   4, -17}, {  87,  -2}, { 100,  -1}, {  99,  -6},
    { -52, -72}, {  67, -11}, {  60,  -7}, {  77, -27},
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
