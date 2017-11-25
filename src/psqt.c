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
    { -13,  -2}, {   3,   0}, {   1,   0}, {  -2,  -3},
    { -16,  -4}, {  -4,  -3}, {  -7,  -5}, {  -8, -11},
    { -18,   1}, { -16,   0}, {  -4,  -9}, {   5, -15},
    { -11,   8}, { -10,   3}, {  -3,  -5}, {   8, -14},
    {  -3,  14}, {   0,  10}, {   6,  -4}, {   4, -16},
    {  -6,  18}, {  -8,  19}, {  -4,  -5}, {  13,  -1},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -42, -49}, { -28, -43}, { -24, -23}, { -21, -26},
    { -21, -37}, { -29, -30}, { -17, -28}, {  -7, -21},
    { -23, -29}, { -11, -22}, {  -2, -13}, {   4, -10},
    { -17, -22}, { -10, -24}, {  -2, -14}, {   1,  -5},
    {  -2, -20}, {   2, -16}, {   4,  -8}, {  18,  -2},
    { -16, -29}, {   4, -22}, {   6, -10}, {  21, -10},
    { -20, -36}, { -15, -28}, {  14, -35}, {  -9, -23},
    {-128, -49}, { -55, -52}, { -80, -57}, { -44, -41},
};

const int BishopPSQT32[32][PHASE_NB] = {
    { -13, -19}, {   0, -17}, { -14, -15}, { -13, -15},
    {  10, -23}, {   7, -20}, {   9, -18}, { -12, -11},
    {  -5, -16}, {   9, -12}, {  -3, -14}, {   0,  -4},
    {  -4, -17}, { -17, -16}, {  -8,  -9}, {   3,  -8},
    { -23, -12}, {  -9, -10}, {  -5, -10}, {   2,  -5},
    {  -9,  -9}, {  -4, -14}, {  -2, -13}, {  -4, -13},
    { -41, -19}, { -19, -16}, { -16, -20}, { -36, -22},
    { -46, -37}, { -44, -25}, { -64, -41}, { -59, -24},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -21, -21}, { -18, -16}, {  -6,  -2}, {   0, -12},
    { -33, -16}, { -20, -21}, { -24, -20}, {  -8, -19},
    { -21, -15}, { -12, -14}, { -18, -17}, { -11, -17},
    { -19,  -5}, { -16,  -6}, { -18, -11}, { -15,  -9},
    { -12,   2}, { -13,  -3}, {  -3,   0}, {  -2,  -3},
    { -11,   2}, {  -8,   1}, {  -2,  -2}, {   0,   0},
    { -12,  -1}, {  -9,  -1}, {   3, -12}, {  -9,  -7},
    { -14,  10}, {  -7,   5}, { -34,  -8}, { -14,   2},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -26, -45}, { -34, -41}, { -30, -28}, { -19, -45},
    { -19, -39}, { -19, -42}, { -21, -62}, { -16, -30},
    { -17, -30}, { -10, -25}, { -21, -26}, { -21, -17},
    { -21, -23}, { -20, -22}, { -26, -21}, { -26,  -4},
    { -22, -11}, { -24,  -9}, { -17, -11}, { -21,   1},
    {  -4, -16}, { -14, -17}, {  -7,  -9}, { -13,   2},
    { -13, -19}, { -47, -12}, {  -9, -23}, { -24,  -2},
    { -40, -44}, { -24, -31}, { -48, -47}, { -32, -26},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  45, -71}, {  60, -57}, {  30, -28}, {  13, -34},
    {  44, -34}, {  47, -21}, {  14,  -6}, {  -6,   0},
    {   7, -26}, {  23, -11}, {   6,   3}, { -12,  11},
    { -17, -24}, {   9,  -7}, {   0,  10}, { -30,  16},
    { -10, -11}, {  14,   3}, {  -5,  13}, { -29,  15},
    {  14,  -2}, {  29,  11}, {   3,  10}, { -30,   6},
    {  10,  -3}, {  15,   6}, { -12,   0}, { -27,   1},
    {   3, -26}, {  17,  -7}, { -23, -25}, { -41, -11},
};

void initalizePSQT(){
    
    int sq, w32, b32;
    
    for (sq = 0; sq < SQUARE_NB; sq++){

        w32 = RelativeSquare32(sq, WHITE);
        b32 = RelativeSquare32(sq, BLACK);

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
