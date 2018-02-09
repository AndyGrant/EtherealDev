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
    { -11,   2}, {  -6,   2}, {   4,   2}, {   9,   2},
    { -10,   0}, {   9,   2}, {   2,   2}, {  -2,  -4},
    { -13,  -2}, {  -1,  -2}, {  -4,  -4}, {  -7, -12},
    { -12,   3}, {  -9,   2}, {   1,  -7}, {   7, -15},
    {  -7,   9}, {  -6,   5}, {   2,  -3}, {  11, -14},
    {   1,  14}, {   5,  11}, {  11,  -3}, {  10, -16},
    {  -8,   7}, { -10,  10}, {  -6, -13}, {  15, -14},
    { -11,   2}, {  -6,   2}, {   4,   2}, {   9,   2},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -20, -26}, {  -7, -23}, {  -7,   1}, {   0,  -3},
    {  -1, -13}, { -10,  -6}, {   0,  -6}, {   9,   0},
    {  -4,  -4}, {   7,   0}, {  12,   8}, {  20,  12},
    {   3,   6}, {   6,   2}, {  15,  11}, {  19,  22},
    {  15,   9}, {  20,  10}, {  21,  20}, {  34,  24},
    {   0,   4}, {  21,   5}, {  21,  20}, {  36,  17},
    {   0,  -8}, {  -3,   0}, {  33, -14}, {   5,   1},
    { -98, -24}, { -42, -22}, { -75, -27}, { -33, -14},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {   4,  -5}, {  16,  -4}, {   0,   1}, {   4,  -1},
    {  26, -10}, {  22,  -8}, {  22,  -4}, {   2,   2},
    {  12,  -2}, {  26,   2}, {  10,   0}, {  15,  12},
    {  12,   0}, {  -6,   1}, {   7,   7}, {  16,   8},
    { -12,   7}, {   5,   7}, {   9,   7}, {  16,  13},
    {   2,   9}, {   8,   4}, {  12,   5}, {  11,   4},
    { -28,   1}, {  -5,   0}, {  -5,  -4}, { -29,  -4},
    { -33, -18}, { -35,  -7}, { -63, -22}, { -59,  -3},
};

const int RookPSQT32[32][PHASE_NB] = {
    {  -9, -19}, {  -7, -13}, {   6,   4}, {  14,  -8},
    { -22, -11}, {  -5, -17}, { -10, -13}, {   7, -14},
    {  -9,  -9}, {   1,  -9}, {  -5, -11}, {   3, -12},
    { -10,   3}, {  -5,   3}, {  -6,  -4}, {  -3,   0},
    {  -3,  10}, {  -3,   5}, {  10,   8}, {  11,   5},
    {  -1,  10}, {   3,  10}, {  11,   5}, {  16,   7},
    {   0,   7}, {   5,   8}, {  21,  -5}, {   5,   2},
    {  -7,  18}, {   5,  12}, { -27,   1}, {  -3,  11},
};

const int QueenPSQT32[32][PHASE_NB] = {
    {  -7, -19}, { -11, -18}, {  -8,   1}, {   4, -26},
    {   5, -14}, {   6, -21}, {   3, -49}, {   7,  -7},
    {   9,  -3}, {  16,  -2}, {   4,  -3}, {   2,   8},
    {   2,   3}, {   3,   2}, {  -3,   3}, {  -3,  21},
    {  -3,  20}, {  -2,  19}, {   4,  13}, {  -1,  28},
    {  15,  11}, {   8,   9}, {  14,  14}, {   5,  29},
    {  13,  10}, { -24,  20}, {  13,  -1}, { -11,  26},
    { -22, -24}, {  -9, -12}, { -35, -34}, { -20,  -8},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  37, -63}, {  48, -50}, {  27, -15}, {   7, -24},
    {  34, -26}, {  39, -15}, {   7,   4}, { -15,   8},
    {  -4, -16}, {  15,  -5}, {  -1,  11}, { -19,  18},
    { -30, -12}, {   1,   1}, {  -6,  18}, { -38,  24},
    { -21,   0}, {  11,  11}, {  -8,  22}, { -32,  23},
    {   6,   9}, {  31,  20}, {   7,  19}, { -27,  14},
    {   6,   8}, {  12,  17}, { -10,   9}, { -20,  10},
    {  -2, -12}, {  14,   6}, { -22, -15}, { -39,  -1},
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
