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
    { -13,  -2}, {   3,   0}, {   2,   0}, {  -1,  -3},
    { -16,  -4}, {  -4,  -3}, {  -6,  -5}, {  -7, -10},
    { -18,   1}, { -16,   0}, {  -4,  -7}, {   5, -14},
    { -11,   7}, { -12,   2}, {  -3,  -5}, {   7, -13},
    {  -3,  12}, {  -1,   8}, {   5,  -4}, {   4, -17},
    {  -3,  21}, {  -6,  23}, {  -2,   1}, {  15,   4},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -42, -47}, { -29, -42}, { -24, -24}, { -20, -24},
    { -22, -38}, { -31, -33}, { -19, -29}, {  -7, -18},
    { -22, -28}, { -11, -21}, {  -1, -11}, {   6,  -9},
    { -17, -21}, { -10, -24}, {  -1, -14}, {   3,  -4},
    {  -2, -20}, {   3, -15}, {   5,  -7}, {  19,  -2},
    { -16, -29}, {   3, -21}, {   9,  -9}, {  21,  -9},
    { -23, -37}, { -18, -28}, {   9, -32}, {  -9, -22},
    {-133, -52}, { -55, -53}, { -77, -61}, { -42, -43},
};

const int BishopPSQT32[32][PHASE_NB] = {
    { -15, -16}, {   1, -13}, { -14, -16}, { -14, -14},
    {  10, -20}, {   6, -18}, {   9, -16}, { -13, -10},
    {  -5, -14}, {  10, -10}, {  -3, -13}, {   2,  -3},
    {  -3, -16}, { -18, -16}, {  -6,  -8}, {   4,  -7},
    { -20, -12}, {  -7,  -8}, {  -5, -10}, {   1,  -5},
    {  -4,  -8}, {  -2, -12}, {  -1, -11}, {  -3, -12},
    { -42, -21}, { -21, -17}, { -19, -20}, { -32, -20},
    { -44, -39}, { -39, -24}, { -56, -43}, { -52, -25},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -21, -22}, { -17, -16}, {  -6,  -3}, {   0, -11},
    { -31, -17}, { -19, -21}, { -23, -21}, {  -7, -18},
    { -18, -14}, { -10, -14}, { -17, -17}, {  -9, -15},
    { -17,  -5}, { -13,  -5}, { -16, -10}, { -13,  -8},
    {  -9,   3}, { -12,  -3}, {  -1,   0}, {  -2,  -1},
    {  -8,   2}, {  -7,   1}, {   0,  -1}, {   1,   0},
    { -10,  -2}, {  -7,  -1}, {   2, -11}, {  -7,  -6},
    {  -8,  10}, {  -4,   5}, { -28,  -8}, {  -7,   4},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -28, -49}, { -33, -43}, { -30, -30}, { -21, -43},
    { -18, -39}, { -19, -41}, { -21, -59}, { -16, -29},
    { -15, -32}, {  -8, -22}, { -19, -23}, { -18, -15},
    { -20, -24}, { -17, -20}, { -24, -21}, { -22,  -4},
    { -19, -16}, { -19, -10}, { -15, -13}, { -17,   0},
    {  -2, -18}, { -10, -18}, {  -6, -10}, { -11,   0},
    { -10, -20}, { -42, -15}, {  -8, -22}, { -20,  -4},
    { -31, -40}, { -18, -29}, { -41, -46}, { -24, -25},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  45, -73}, {  61, -58}, {  30, -28}, {  11, -34},
    {  45, -33}, {  46, -20}, {  14,  -7}, {  -6,   0},
    {   8, -26}, {  23, -10}, {   6,   2}, { -12,  12},
    { -15, -25}, {   9,  -6}, {   0,  10}, { -28,  18},
    { -10, -12}, {  11,   2}, {  -7,  12}, { -29,  17},
    {  13,  -1}, {  25,  11}, {   0,  10}, { -32,   8},
    {   5, -11}, {  12,   3}, { -16,  -1}, { -33,   0},
    {  -6, -43}, {  10, -11}, { -32, -31}, { -48, -16},
};

void initalizePSQT(){
    
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
