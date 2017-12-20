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
    { -15,  -3}, {   4,   0}, {   0,   2}, {  -4, -11},
    { -15,  -3}, {   0,  -2}, {  -6,  -3}, { -10,  -8},
    { -16,   3}, {  -4,   1}, {   0,  -6}, {   4, -15},
    { -11,   5}, {   0,   4}, {   4,  -4}, {   8, -12},
    {  10,   0}, {  17,   2}, {  30,  -3}, {  15, -13},
    {  18, -44}, {  20, -12}, {  36, -12}, {  40, -13},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -41, -93}, { -22, -52}, { -30, -50}, { -20, -39},
    { -19, -47}, { -28, -38}, { -21, -41}, {  -1, -33},
    { -24, -42}, {  -7, -32}, {  -8, -26}, {   6, -17},
    { -13, -25}, {  -2, -22}, {  -5, -14}, {   0,  -7},
    {   0, -17}, {  -1, -14}, {   8,  -3}, {   8,  -1},
    { -14, -12}, {   6, -19}, {  17,  -3}, {  31,  -4},
    { -26, -23}, { -13, -17}, {  26, -27}, {  14, -20},
    {-103, -35}, { -30, -16}, { -55, -26}, { -12,  -6},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {   0, -25}, {  -5, -23}, { -20, -19}, { -14, -30},
    {  10, -35}, {   5, -24}, {   4, -23}, { -10, -16},
    {  -3, -22}, {  14, -20}, {  -1, -15}, {   0, -15},
    {   1, -15}, { -11, -12}, { -11,  -7}, {   5,  -9},
    { -28, -12}, { -15,  -4}, {  -6,  -8}, {   9,  -6},
    {  -8, -11}, {  -2, -11}, {   8,  -4}, {   1,  -8},
    { -30, -14}, { -18, -11}, {  -7, -16}, { -13,  -7},
    { -40, -22}, { -25, -19}, { -46, -21}, { -48, -18},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -19, -22}, { -21, -20}, {  -9, -10}, {  -2, -20},
    { -43, -25}, { -28, -25}, { -27, -25}, { -14, -28},
    { -30, -15}, { -18, -14}, { -19, -17}, { -17, -17},
    { -17,   1}, {  -5,   4}, { -13,   0}, {  -9,  -3},
    {  -1,  11}, {  -3,   8}, {  15,   8}, {  14,   7},
    {   1,   8}, {   9,   3}, {  19,   5}, {  17,   7},
    {  -2,   4}, { -11,  -1}, {  12,  -7}, {  12,   1},
    {   5,  24}, {   7,  13}, {  -8,   8}, {   4,  16},
};
const int QueenPSQT32[32][PHASE_NB] = {
    { -22, -50}, { -32, -48}, { -36, -34}, { -16, -50},
    { -19, -49}, { -16, -48}, {  -9, -60}, { -16, -31},
    { -12, -35}, {  -3, -26}, { -19, -20}, { -23, -16},
    { -16, -15}, { -17, -16}, { -25, -15}, { -28,   3},
    { -23,  -4}, { -25,   1}, { -19,   1}, { -26,   7},
    { -11,  -7}, { -10,  -3}, {  -1,   0}, { -14,   6},
    { -15,  -2}, { -49,   5}, {  -2, -12}, { -19,   5},
    { -17, -34}, { -16, -26}, { -25, -32}, { -28, -24},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  31, -75}, {  61, -53}, {  28, -32}, {  17, -41},
    {  34, -33}, {  37, -20}, {   9,  -5}, {  -7,  -1},
    {  -4, -17}, {  27,  -6}, {  13,   5}, {  -4,  13},
    {   0, -21}, {  32,   2}, {  13,  13}, { -30,  18},
    {   9, -15}, {  59,  12}, {   8,  10}, { -56,   0},
    {  35,  -1}, {  65,   7}, {  49,   6}, { -31, -14},
    {   4, -23}, {  69,   5}, {  73,  12}, {  58,   1},
    { -70, -88}, {  87,  -2}, {  43,   1}, {  54,  -5},
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
