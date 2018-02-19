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
    { -18,  -2}, {   6,  -1}, {  -9,   0}, {  -9, -11},
    { -17,  -3}, {  -2,  -3}, { -10,  -7}, { -10, -12},
    { -16,   0}, {  -2,   0}, {   1, -10}, {   2, -19},
    {  -8,   4}, {   3,   3}, {   2,  -5}, {   4, -17},
    {   0,   5}, {  10,   7}, {  20,  -1}, {  15, -13},
    { -24, -22}, { -13,  -6}, {  12, -14}, {  18, -20},
    { -15,   0}, { -10,   0}, {   0,   0}, {   5,   0},
};

const int KnightPSQT32[32][PHASE_NB] = {
    { -44, -77}, { -21, -56}, { -32, -49}, { -18, -47},
    { -16, -60}, { -18, -44}, { -18, -51}, {  -6, -41},
    { -21, -49}, {  -6, -45}, { -13, -36}, {   0, -27},
    { -10, -30}, {  -2, -26}, {  -3, -18}, {   0, -14},
    {   0, -22}, {   2, -21}, {   5,  -7}, {   8,  -6},
    { -26, -16}, {   3, -23}, {  10,  -7}, {  20,  -8},
    { -19, -36}, { -21, -24}, {  43, -48}, {   8, -33},
    { -91, -50}, { -51, -32}, { -84, -32}, { -45, -24},
};

const int BishopPSQT32[32][PHASE_NB] = {
    {   0, -41}, {  -5, -41}, { -23, -23}, { -12, -37},
    {  10, -47}, {   5, -35}, {   0, -32}, { -11, -26},
    {   0, -34}, {   8, -30}, {  -4, -21}, {  -3, -17},
    {   2, -26}, {  -8, -21}, {  -8, -13}, {   5, -13},
    { -28, -16}, {  -5, -15}, {  -8, -12}, {   3,  -7},
    { -17, -17}, { -11, -15}, {   4,  -9}, {   1, -12},
    { -47, -16}, {  -6, -21}, {  -6, -26}, { -27, -17},
    { -34, -31}, { -56, -25}, { -87, -25}, { -91, -18},
};

const int RookPSQT32[32][PHASE_NB] = {
    { -20, -29}, { -23, -26}, {  -9, -16}, {  -7, -26},
    { -41, -26}, { -24, -30}, { -23, -27}, { -14, -31},
    { -30, -19}, { -16, -22}, { -20, -22}, { -18, -23},
    { -23,  -8}, { -17,  -4}, { -16,  -9}, { -14,  -9},
    { -16,   1}, { -15,   0}, {   5,  -2}, {   2,   0},
    { -16,   0}, {  -5,  -1}, {   6,  -2}, {   8,  -1},
    {  -7,   2}, { -16,   0}, {  19, -13}, {  10,  -2},
    { -18,   8}, {  -9,   4}, { -32,   0}, { -10,   6},
};

const int QueenPSQT32[32][PHASE_NB] = {
    { -37, -66}, { -43, -64}, { -43, -48}, { -28, -70},
    { -28, -71}, { -25, -72}, { -21, -82}, { -27, -52},
    { -24, -53}, { -15, -52}, { -27, -41}, { -32, -38},
    { -27, -35}, { -25, -34}, { -31, -31}, { -34, -17},
    { -29, -16}, { -31, -15}, { -26, -18}, { -37,  -8},
    { -26, -29}, { -17, -23}, {  -7, -26}, { -20, -10},
    { -26, -27}, { -47, -19}, {  -5, -39}, { -36,  -8},
    { -26, -40}, { -24, -37}, { -24, -43}, { -40, -28},
};

const int KingPSQT32[32][PHASE_NB] = {
    {  43, -64}, {  53, -52}, {  28, -22}, {  12, -27},
    {  41, -31}, {  33, -26}, {   6,  -3}, { -11,   0},
    {   3, -17}, {  27, -14}, {  14,   2}, {  -3,  12},
    {  -3, -20}, {  34,  -4}, {   9,  10}, { -31,  20},
    {   8, -16}, {  57,   2}, {  13,  13}, { -30,  11},
    {  31, -12}, {  66,   0}, {  52,   8}, {   9,   0},
    {  -6, -20}, {  60,   0}, {  64,   3}, {  55,   2},
    { -27, -73}, {  85, -28}, {  40, -10}, {  31, -16},
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
