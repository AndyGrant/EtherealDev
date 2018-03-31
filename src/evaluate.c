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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "bitboards.h"
#include "bitutils.h"
#include "castle.h"
#include "evaluate.h"
#include "magics.h"
#include "masks.h"
#include "movegen.h"
#include "piece.h"
#include "square.h"
#include "transposition.h"
#include "types.h"

#ifdef TUNE
    const int TEXEL = 1;
    const int TRACE = 1;
    const EvalTrace EmptyTrace;
    EvalTrace T;
#else
    const int TEXEL = 0;
    const int TRACE = 0;
    EvalTrace T;
#endif

// Definition of Values for each Piece type


const int PawnValue[PHASE_NB]   = { 100, 122};

const int KnightValue[PHASE_NB] = { 461, 391};

const int BishopValue[PHASE_NB] = { 470, 415};

const int RookValue[PHASE_NB]   = { 638, 715};

const int QueenValue[PHASE_NB]  = {1290,1332};

const int KingValue[PHASE_NB]   = { 166, 166};

const int NoneValue[PHASE_NB]   = {   0,   0};

const int PawnIsolated[PHASE_NB] = {  -4,  -5};

const int PawnStacked[PHASE_NB] = { -10, -32};

const int PawnBackwards[2][PHASE_NB] = { {   6,  -3}, { -12, -11} };

const int PawnConnected32[32][PHASE_NB] = {
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
    {   0, -16}, {   6,   1}, {   3,  -3}, {   5,  19},
    {   7,   0}, {  21,   0}, {  15,   8}, {  17,  22},
    {   6,   0}, {  19,   4}, {  15,   8}, {  17,  17},
    {   6,  13}, {  20,  20}, {  19,  24}, {  38,  24},
    {  25,  55}, {  29,  64}, {  72,  61}, {  57,  76},
    { 106, -19}, { 180,  10}, { 222,  22}, { 208,  79},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightAttackedByPawn[PHASE_NB] = { -50, -34};

const int KnightRammedPawns[PHASE_NB] = {   1,   6};

const int KnightOutpost[2][PHASE_NB] = { {  19, -35}, {  36,   6} };

const int KnightMobility[9][PHASE_NB] = {
    { -87, -97}, { -37, -88}, { -18, -42},
    {  -6, -12}, {   2, -14}, {   7,   0},
    {  18,  -1}, {  32,  -1}, {  48, -32},
};

const int BishopPair[PHASE_NB] = {  43,  69};

const int BishopRammedPawns[PHASE_NB] = { -10,  -6};

const int BishopAttackedByPawn[PHASE_NB] = { -52, -38};

const int BishopOutpost[2][PHASE_NB] = { {  19, -16}, {  52,  -9} };

const int BishopMobility[14][PHASE_NB] = {
    { -63,-124}, { -48, -66}, { -19, -46}, {  -5, -21},
    {   4,  -7}, {  17,   0}, {  22,   8}, {  28,   4},
    {  29,  10}, {  36,   3}, {  41,   4}, {  50, -14},
    {  47,  -1}, {  39, -35},
};

const int RookFile[2][PHASE_NB] = { {  13,   1}, {  40,  -8} };

const int RookOnSeventh[PHASE_NB] = {   0,  24};

const int RookMobility[15][PHASE_NB] = {
    {-157,-101}, { -72,-120}, { -16, -67}, {  -9, -26},
    {  -7,  -3}, {  -7,  14}, {  -7,  25}, {  -3,  31},
    {   0,  36}, {   6,  36}, {  10,  42}, {  18,  48},
    {  20,  50}, {  25,  47}, {  15,  44},
};

const int QueenChecked[PHASE_NB] = { -38, -32};

const int QueenCheckedByPawn[PHASE_NB] = { -52, -46};

const int QueenMobility[28][PHASE_NB] = {
    { -60,-260}, {-261,-317}, { -42,-198}, { -37,-191},
    { -15,-123}, { -25, -63}, { -15, -89}, { -18, -84},
    { -14, -59}, { -10, -53}, {  -8, -27}, {  -5, -29},
    {  -5, -15}, {  -1, -10}, {   1,  -6}, {   0,   4},
    {   3,  16}, {   0,  15}, {  10,  25}, {  -1,  25},
    {   3,  27}, {  18,  29}, {  19,  11}, {  33,  15},
    {  49,  23}, {  53,   0}, { -17,  -5}, {  19,   7},
};

const int KingDefenders[12][PHASE_NB] = {
    { -39,  -4}, { -20,   5}, {   0,   0}, {  10,   0},
    {  24,  -2}, {  35,   2}, {  40,  10}, {  28,-154},
    {  12,   6}, {  12,   6}, {  12,   6}, {  12,   6},
};

const int KingShelter[2][FILE_NB][RANK_NB][PHASE_NB] = {
  {{{ -16,  17}, {   6,  -9}, {  15,   2}, {  22,   3}, {  10,   3}, {   0,   0}, {   0,   0}, { -33,   1}},
   {{   3,   7}, {  16,  -6}, {  15,  -9}, {   0, -12}, { -27,  -2}, {   0,   0}, {   0,   0}, { -32,   0}},
   {{  12,  14}, {   8,  -1}, { -19,   0}, {  -8,  -1}, { -29,  -3}, {   0,   0}, {   0,   0}, { -15,   1}},
   {{   9,  29}, {  15,   0}, {  -3,  -8}, {  19, -14}, {  16, -34}, {   0,   0}, {   0,   0}, {  -2,   0}},
   {{ -15,  18}, {   0,   2}, { -28,   1}, { -12,   3}, { -33, -15}, {   0,   0}, {   0,   0}, { -15,   0}},
   {{  22,   2}, {  17,  -4}, { -20,  -1}, {  -2, -21}, {   5, -32}, {   0,   0}, {   0,   0}, { -13,   0}},
   {{  19,   2}, {   2,  -9}, { -28,  -8}, { -20, -13}, { -22, -17}, {   0,   0}, {   0,   0}, { -29,   8}},
   {{ -16,  -3}, {   0,  -8}, {   4,   0}, {   2,   3}, { -12,  12}, {   0,   0}, {   0,   0}, { -18,  15}}},
  {{{   0,   0}, {   3, -16}, {   7, -15}, { -52,  16}, {  14, -10}, {   0,   0}, {   0,   0}, { -55,  10}},
   {{   0,   0}, {  17,  -6}, {   7,  -6}, {  -1,  -7}, {   8, -29}, {   0,   0}, {   0,   0}, { -46,   3}},
   {{   0,   0}, {  23,   0}, {   0,  -6}, {  19, -26}, {  21,  -5}, {   0,   0}, {   0,   0}, { -21,  -1}},
   {{   0,   0}, {  -5,   8}, { -11,  13}, { -15,   2}, { -25,   0}, {   0,   0}, {   0,   0}, { -26,   0}},
   {{   0,   0}, {   4,   5}, {   8,  -7}, {  23,  -5}, {   9, -21}, {   0,   0}, {   0,   0}, {  -5,  -2}},
   {{   0,   0}, {  10,   1}, { -15,  -2}, { -15, -13}, {  22, -33}, {   0,   0}, {   0,   0}, { -30,   0}},
   {{   0,   0}, {  12,  -1}, {  -2,   0}, { -20,  -5}, { -20, -14}, {   0,   0}, {   0,   0}, { -45,  13}},
   {{   0,   0}, {   8, -27}, {  10, -15}, { -23,   0}, { -25,  -4}, {   0,   0}, {   0,   0}, { -45,  16}}},
};

const int PassedPawn[2][2][RANK_NB][PHASE_NB] = {
  {{{   0,   0}, { -29, -28}, { -23,   8}, { -12,  -1}, {  20,   0}, {  59,  -5}, { 154,  33}, {   0,   0}},
   {{   0,   0}, {  -2,   2}, { -14,  23}, { -12,  36}, {   7,  45}, {  70,  63}, { 192, 130}, {   0,   0}}},
  {{{   0,   0}, {  -7,  13}, { -11,   7}, {  -9,  27}, {  27,  33}, {  79,  64}, { 218, 150}, {   0,   0}},
   {{   0,   0}, {  -8,   8}, { -13,  17}, { -19,  53}, {  -9, 111}, {  34, 206}, { 121, 371}, {   0,   0}}},
};

const int* PieceValues[8] = {
      PawnValue, KnightValue, BishopValue,   RookValue,
     QueenValue,   KingValue,   NoneValue,   NoneValue,
};



// Definition of evaluation terms related to Kings

int KingSafety[64]; // Defined by the Polynomial below

const double KingPolynomial[6] = {
    0.00000011, -0.00009948,  0.00797308, 
    0.03141319,  2.18429452, -3.33669140
};

// Definition of evaluation terms related to general properties

const int Tempo[COLOUR_NB][PHASE_NB] = { {  25,  12}, { -25, -12} };


int evaluateBoard(Board* board, EvalInfo* ei, PawnKingTable* pktable){
    
    int mg, eg, phase, eval;
    
    // evaluateDraws handles obvious drawn positions
    ei->positionIsDrawn = evaluateDraws(board);
    if (ei->positionIsDrawn) return 0;
    
    // Setup and perform the evaluation of all pieces
    initializeEvalInfo(ei, board, pktable);
    evaluatePieces(ei, board);
    
    // Store a new PawnKing entry if we did not have one (and are not doing Texel)
    if (ei->pkentry == NULL && !TEXEL){
        mg = ei->pawnKingMidgame[WHITE] - ei->pawnKingMidgame[BLACK];
        eg = ei->pawnKingEndgame[WHITE] - ei->pawnKingEndgame[BLACK];
        storePawnKingEntry(pktable, board->pkhash, ei->passedPawns, mg, eg);
    }
    
    // Otherwise, fetch the PawnKing evaluation (if we are not doing Texel)
    else if (!TEXEL){
        ei->pawnKingMidgame[WHITE] = ei->pkentry->mg;
        ei->pawnKingEndgame[WHITE] = ei->pkentry->eg;
    }
        
    // Combine evaluation terms for the mid game
    mg = board->midgame + ei->midgame[WHITE] - ei->midgame[BLACK]
       + ei->pawnKingMidgame[WHITE] - ei->pawnKingMidgame[BLACK] + Tempo[board->turn][MG];
       
    // Combine evaluation terms for the end game
    eg = board->endgame + ei->endgame[WHITE] - ei->endgame[BLACK]
       + ei->pawnKingEndgame[WHITE] - ei->pawnKingEndgame[BLACK] + Tempo[board->turn][EG];
       
    // Calcuate the game phase based on remaining material (Fruit Method)
    phase = 24 - popcount(board->pieces[QUEEN]) * 4
               - popcount(board->pieces[ROOK]) * 2
               - popcount(board->pieces[KNIGHT] | board->pieces[BISHOP]);
    phase = (phase * 256 + 12) / 24;
          
    // Compute the interpolated evaluation
    eval  = (mg * (256 - phase) + eg * phase) / 256;
    
    // Return the evaluation relative to the side to move
    return board->turn == WHITE ? eval : -eval;
}

int evaluateDraws(Board* board){
    
    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];
    uint64_t pawns   = board->pieces[PAWN];
    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK];
    uint64_t queens  = board->pieces[QUEEN];
    uint64_t kings   = board->pieces[KING];
    
    // Unlikely to have a draw if we have pawns, rooks, or queens left
    if (pawns | rooks | queens)
        return 0;
    
    // Check for King Vs. King
    if (kings == (white | black)) return 1;
    
    if ((white & kings) == white){
        // Check for King Vs King and Knight/Bishop
        if (popcount(black & (knights | bishops)) <= 1) return 1;
        
        // Check for King Vs King and two Knights
        if (popcount(black & knights) == 2 && (black & bishops) == 0ull) return 1;
    }
    
    if ((black & kings) == black){
        // Check for King Vs King and Knight/Bishop
        if (popcount(white & (knights | bishops)) <= 1) return 1;
        
        // Check for King Vs King and two Knights
        if (popcount(white & knights) == 2 && (white & bishops) == 0ull) return 1;
    }
    
    return 0;
}

void evaluatePieces(EvalInfo* ei, Board* board){
    
    evaluatePawns(ei, board, WHITE);
    evaluatePawns(ei, board, BLACK);
    
    evaluateKnights(ei, board, WHITE);
    evaluateKnights(ei, board, BLACK);
    
    evaluateBishops(ei, board, WHITE);
    evaluateBishops(ei, board, BLACK);
    
    evaluateRooks(ei, board, WHITE);
    evaluateRooks(ei, board, BLACK);
    
    evaluateQueens(ei, board, WHITE);
    evaluateQueens(ei, board, BLACK);
    
    evaluateKings(ei, board, WHITE);
    evaluateKings(ei, board, BLACK);
    
    evaluatePassedPawns(ei, board, WHITE);
    evaluatePassedPawns(ei, board, BLACK);
}

void evaluatePawns(EvalInfo* ei, Board* board, int colour){
    
    const int forward = (colour == WHITE) ? 8 : -8;
    
    int sq, semi;
    uint64_t pawns, myPawns, tempPawns, enemyPawns, attacks;
    
    // Update the attacks array with the pawn attacks. We will use this to
    // determine whether or not passed pawns may advance safely later on.
    // It is also used to compute pawn threats against minors and majors
    ei->attackedBy2[colour]     = ei->pawnAttacks[colour] & ei->attacked[colour];
    ei->attacked[colour]       |= ei->pawnAttacks[colour];
    ei->attackedNoQueen[colour] = ei->pawnAttacks[colour];
    
    // Update the attack counts for our pawns. We will not count squares twice
    // even if they are attacked by two pawns. Also, we do not count pawns
    // torwards our attackers counts, which is used to decide when to look
    // at the King Safety of a position.
    attacks = ei->pawnAttacks[colour] & ei->kingAreas[!colour];
    ei->attackCounts[colour] += popcount(attacks);
    
    // The pawn table holds the rest of the eval information we will calculate
    if (ei->pkentry != NULL) return;
    
    pawns = board->pieces[PAWN];
    myPawns = tempPawns = pawns & board->colours[colour];
    enemyPawns = pawns & board->colours[!colour];
    
    // Evaluate each pawn (but not for being passed)
    while (tempPawns != 0ull){
        
        // Pop off the next pawn
        sq = poplsb(&tempPawns);
        
        if (TRACE) T.pawnCounts[colour]++;
        if (TRACE) T.pawnPSQT[colour][sq]++;
        
        // Save the fact that this pawn is passed
        if (!(PassedPawnMasks[colour][sq] & enemyPawns))
            ei->passedPawns |= (1ull << sq);
        
        // Apply a penalty if the pawn is isolated
        if (!(IsolatedPawnMasks[sq] & tempPawns)){
            ei->pawnKingMidgame[colour] += PawnIsolated[MG];
            ei->pawnKingEndgame[colour] += PawnIsolated[EG];
            if (TRACE) T.pawnIsolated[colour]++;
        }
        
        // Apply a penalty if the pawn is stacked
        if (Files[File(sq)] & tempPawns){
            ei->pawnKingMidgame[colour] += PawnStacked[MG];
            ei->pawnKingEndgame[colour] += PawnStacked[EG];
            if (TRACE) T.pawnStacked[colour]++;
        }
        
        // Apply a penalty if the pawn is backward
        if (   !(PassedPawnMasks[!colour][sq] & myPawns)
            &&  (ei->pawnAttacks[!colour] & (1ull << (sq + forward)))){
                
            semi = !(Files[File(sq)] & enemyPawns);
            
            ei->pawnKingMidgame[colour] += PawnBackwards[semi][MG];
            ei->pawnKingEndgame[colour] += PawnBackwards[semi][EG];
            if (TRACE) T.pawnBackwards[colour][semi]++;
        }
        
        // Apply a bonus if the pawn is connected and not backward
        else if (PawnConnectedMasks[colour][sq] & myPawns){
            ei->pawnKingMidgame[colour] += PawnConnected32[relativeSquare32(sq, colour)][MG];
            ei->pawnKingEndgame[colour] += PawnConnected32[relativeSquare32(sq, colour)][EG];
            if (TRACE) T.pawnConnected[colour][sq]++;
        }
    }
}

void evaluateKnights(EvalInfo* ei, Board* board, int colour){
    
    int sq, defended, count;
    uint64_t tempKnights, enemyPawns, attacks; 
    
    tempKnights = board->pieces[KNIGHT] & board->colours[colour];
    enemyPawns = board->pieces[PAWN] & board->colours[!colour];
    
    // Evaluate each knight
    while (tempKnights){
        
        // Pop off the next knight
        sq = poplsb(&tempKnights);
        
        if (TRACE) T.knightCounts[colour]++;
        if (TRACE) T.knightPSQT[colour][sq]++;
        
        // Update the attacks array with the knight attacks. We will use this to
        // determine whether or not passed pawns may advance safely later on.
        attacks = knightAttacks(sq, ~0ull);
        ei->attackedBy2[colour] |= ei->attacked[colour] & attacks;
        ei->attacked[colour] |= attacks;
        ei->attackedNoQueen[colour] |= attacks;
        
        // Apply a bonus for the knight based on number of rammed pawns
        count = popcount(ei->rammedPawns[colour]);
        ei->midgame[colour] += count * KnightRammedPawns[MG];
        ei->endgame[colour] += count * KnightRammedPawns[EG];
        if (TRACE) T.knightRammedPawns[colour] += count;
        
        // Apply a penalty if the knight is being attacked by a pawn
        if (ei->pawnAttacks[!colour] & (1ull << sq)){
            ei->midgame[colour] += KnightAttackedByPawn[MG];
            ei->endgame[colour] += KnightAttackedByPawn[EG];
            if (TRACE) T.knightAttackedByPawn[colour]++;
        }
        
        // Apply a bonus if the knight is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the knight.
        if (    (OutpostRanks[colour] & (1ull << sq))
            && !(OutpostSquareMasks[colour][sq] & enemyPawns)){
                
            defended = !!(ei->pawnAttacks[colour] & (1ull << sq));
            
            ei->midgame[colour] += KnightOutpost[defended][MG];
            ei->endgame[colour] += KnightOutpost[defended][EG];
            if (TRACE) T.knightOutpost[colour][defended]++;
        }
        
        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount((ei->mobilityAreas[colour] & attacks));
        ei->midgame[colour] += KnightMobility[count][MG];
        ei->endgame[colour] += KnightMobility[count][EG];
        if (TRACE) T.knightMobility[colour][count]++;
        
        // Update the attack and attacker counts for the
        // knight for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += 2 * popcount(attacks);
            ei->attackerCounts[colour] += 1;
        }
    }
}

void evaluateBishops(EvalInfo* ei, Board* board, int colour){
    
    int sq, defended, count;
    uint64_t tempBishops, enemyPawns, attacks;
    
    tempBishops = board->pieces[BISHOP] & board->colours[colour];
    enemyPawns = board->pieces[PAWN] & board->colours[!colour];
    
    // Apply a bonus for having a pair of bishops
    if ((tempBishops & WHITE_SQUARES) && (tempBishops & BLACK_SQUARES)){
        ei->midgame[colour] += BishopPair[MG];
        ei->endgame[colour] += BishopPair[EG];
        if (TRACE) T.bishopPair[colour]++;
    }
    
    // Evaluate each bishop
    while (tempBishops){
        
        // Pop off the next Bishop
        sq = poplsb(&tempBishops);
        
        if (TRACE) T.bishopCounts[colour]++;
        if (TRACE) T.bishopPSQT[colour][sq]++;
        
        // Update the attacks array with the bishop attacks. We will use this to
        // determine whether or not passed pawns may advance safely later on.
        attacks = bishopAttacks(sq, ei->occupiedMinusBishops[colour], ~0ull);
        ei->attackedBy2[colour] |= ei->attacked[colour] & attacks;
        ei->attacked[colour] |= attacks;
        ei->attackedNoQueen[colour] |= attacks;
        
        // Apply a penalty for the bishop based on number of rammed pawns
        // of our own colour, which reside on the same shade of square as the bishop
        count = popcount(ei->rammedPawns[colour] & (((1ull << sq) & WHITE_SQUARES ? WHITE_SQUARES : BLACK_SQUARES)));
        ei->midgame[colour] += count * BishopRammedPawns[MG];
        ei->endgame[colour] += count * BishopRammedPawns[EG];
        if (TRACE) T.bishopRammedPawns[colour] += count;
        
        // Apply a penalty if the bishop is being attacked by a pawn
        if (ei->pawnAttacks[!colour] & (1ull << sq)){
            ei->midgame[colour] += BishopAttackedByPawn[MG];
            ei->endgame[colour] += BishopAttackedByPawn[EG];
            if (TRACE) T.bishopAttackedByPawn[colour]++;
        }
        
        // Apply a bonus if the bishop is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the bishop.
        if (    (OutpostRanks[colour] & (1ull << sq))
            && !(OutpostSquareMasks[colour][sq] & enemyPawns)){
                
            defended = !!(ei->pawnAttacks[colour] & (1ull << sq));
            
            ei->midgame[colour] += BishopOutpost[defended][MG];
            ei->endgame[colour] += BishopOutpost[defended][EG];
            if (TRACE) T.bishopOutpost[colour][defended]++;
        }
        
        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount((ei->mobilityAreas[colour] & attacks));
        ei->midgame[colour] += BishopMobility[count][MG];
        ei->endgame[colour] += BishopMobility[count][EG];
        if (TRACE) T.bishopMobility[colour][count]++;
        
        // Update the attack and attacker counts for the
        // bishop for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += 2 * popcount(attacks);
            ei->attackerCounts[colour] += 1;
        }
    }
}

void evaluateRooks(EvalInfo* ei, Board* board, int colour){
    
    int sq, open, mobilityCount;
    uint64_t attacks;
    
    uint64_t myPawns    = board->pieces[PAWN] & board->colours[ colour];
    uint64_t enemyPawns = board->pieces[PAWN] & board->colours[!colour];
    uint64_t tempRooks  = board->pieces[ROOK] & board->colours[ colour];
    uint64_t enemyKings = board->pieces[KING] & board->colours[!colour];
    
    // Evaluate each rook
    while (tempRooks){
        
        // Pop off the next rook
        sq = poplsb(&tempRooks);
        
        if (TRACE) T.rookCounts[colour]++;
        if (TRACE) T.rookPSQT[colour][sq]++;
        
        // Update the attacks array with the rooks attacks. We will use this to
        // determine whether or not passed pawns may advance safely later on.
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[colour], ~0ull);
        ei->attackedBy2[colour] |= ei->attacked[colour] & attacks;
        ei->attacked[colour] |= attacks;
        ei->attackedNoQueen[colour] |= attacks;
        
        // Rook is on a semi-open file if there are no
        // pawns of the Rook's colour on the file. If
        // there are no pawns at all, it is an open file
        if (!(myPawns & Files[File(sq)])){
            open = !(enemyPawns & Files[File(sq)]);
            ei->midgame[colour] += RookFile[open][MG];
            ei->endgame[colour] += RookFile[open][EG];
            if (TRACE) T.rookFile[colour][open]++;
        }
        
        // Rook gains a bonus for being located on seventh rank relative to its
        // colour so long as the enemy king is on the last two ranks of the board
        if (   Rank(sq) == (colour == BLACK ? 1 : 6)
            && Rank(relativeSquare(getlsb(enemyKings), colour)) >= 6){
            ei->midgame[colour] += RookOnSeventh[MG];
            ei->endgame[colour] += RookOnSeventh[EG];
            if (TRACE) T.rookOnSeventh[colour]++;
        }
        
        // Apply a bonus (or penalty) based on the mobility of the rook
        mobilityCount = popcount((ei->mobilityAreas[colour] & attacks));
        ei->midgame[colour] += RookMobility[mobilityCount][MG];
        ei->endgame[colour] += RookMobility[mobilityCount][EG];
        if (TRACE) T.rookMobility[colour][mobilityCount]++;
        
        // Update the attack and attacker counts for the
        // rook for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += 3 * popcount(attacks);
            ei->attackerCounts[colour] += 1;
        }
    }
}

void evaluateQueens(EvalInfo* ei, Board* board, int colour){
    
    int sq, mobilityCount;
    uint64_t tempQueens, attacks;
    
    tempQueens = board->pieces[QUEEN] & board->colours[colour];
    
    // Evaluate each queen
    while (tempQueens){
        
        // Pop off the next queen
        sq = poplsb(&tempQueens);
        
        if (TRACE) T.queenCounts[colour]++;
        if (TRACE) T.queenPSQT[colour][sq]++;
        
        // Update the attacks array with the rooks attacks. We will use this to
        // determine whether or not passed pawns may advance safely later on.
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[colour], ~0ull)
                | bishopAttacks(sq, ei->occupiedMinusBishops[colour], ~0ull);
        ei->attackedBy2[colour] |= ei->attacked[colour] & attacks;
        ei->attacked[colour] |= attacks;
            
        // Apply a penalty if the queen is under an attack threat
        if ((1ull << sq) & ei->attackedNoQueen[!colour]){
            ei->midgame[colour] += QueenChecked[MG];
            ei->endgame[colour] += QueenChecked[EG];
            if (TRACE) T.queenChecked[colour]++;
        }
        
        // Apply a penalty if the queen is under attack by a pawn
        if ((1ull << sq) & ei->pawnAttacks[!colour]){
            ei->midgame[colour] += QueenCheckedByPawn[MG];
            ei->endgame[colour] += QueenCheckedByPawn[EG];
            if (TRACE) T.queenCheckedByPawn[colour]++;
        }
            
        // Apply a bonus (or penalty) based on the mobility of the queen
        mobilityCount = popcount((ei->mobilityAreas[colour] & attacks));
        ei->midgame[colour] += QueenMobility[mobilityCount][MG];
        ei->endgame[colour] += QueenMobility[mobilityCount][EG];
        if (TRACE) T.queenMobility[colour][mobilityCount]++;
        
        // Update the attack and attacker counts for the queen for use in
        // the king safety calculation. We could the Queen as two attacking
        // pieces. This way King Safety is always used with the Queen attacks
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += 4 * popcount(attacks);
            ei->attackerCounts[colour] += 2;
        }
    }
}

void evaluateKings(EvalInfo* ei, Board* board, int colour){
    
    int defenderCounts, attackCounts;
    
    int file, kingFile, kingRank, kingSq, distance;
    
    uint64_t filePawns;
    
    uint64_t myPawns = board->pieces[PAWN] & board->colours[colour];
    uint64_t myKings = board->pieces[KING] & board->colours[colour];
    
    uint64_t myDefenders  = (board->pieces[PAWN  ] & board->colours[colour])
                          | (board->pieces[KNIGHT] & board->colours[colour])
                          | (board->pieces[BISHOP] & board->colours[colour]);
                          
    kingSq = getlsb(myKings);
    kingFile = File(kingSq);
    kingRank = Rank(kingSq);
    
    // For Tuning Piece Square Table for Kings
    if (TRACE) T.kingPSQT[colour][kingSq]++;
    
    // Bonus for our pawns and minors sitting within our king area
    defenderCounts = popcount(myDefenders & ei->kingAreas[colour]);
    ei->midgame[colour] += KingDefenders[defenderCounts][MG];
    ei->endgame[colour] += KingDefenders[defenderCounts][EG];
    if (TRACE) T.kingDefenders[colour][defenderCounts]++;
    
    // If we have two or more threats to our king area, we will apply a penalty
    // based on the number of squares attacked, and the strength of the attackers
    if (ei->attackerCounts[!colour] >= 2){
        
        attackCounts = ei->attackCounts[!colour];
        
        // Add an extra two attack counts per missing pawn in the king area.
        attackCounts += 6 - 2 * popcount(myPawns & ei->kingAreas[colour]);
        
        // Scale down attack count if there are no enemy queens
        if (!(board->colours[!colour] & board->pieces[QUEEN]))
            attackCounts *= .25;
    
        ei->midgame[colour] -= KingSafety[MIN(63, MAX(0, attackCounts))];
    }
    
    // Pawn Shelter evaluation is stored in the PawnKing evaluation table
    if (ei->pkentry != NULL) return;
    
    // Evaluate Pawn Shelter. We will look at the King's file and any adjacent files
    // to the King's file. We evaluate the distance between the king and the most backward
    // pawn. We will not look at pawns behind the king, and will consider that as having
    // no pawn on the file. No pawn on a file is used with distance equals 7, as no pawn
    // can ever be a distance of 7 from the king. Different bonus is in order when we are
    // looking at the file on which the King sits.
    
    for (file = MAX(0, kingFile - 1); file <= MIN(7, kingFile + 1); file++){
        
        filePawns = myPawns & Files[file] & RanksAtOrAboveMasks[colour][kingRank];
        
        distance = filePawns ? 
                   colour == WHITE ? Rank(getlsb(filePawns)) - kingRank
                                   : kingRank - Rank(getmsb(filePawns))
                                   : 7;

        ei->pawnKingMidgame[colour] += KingShelter[file == kingFile][file][distance][MG];
        ei->pawnKingEndgame[colour] += KingShelter[file == kingFile][file][distance][EG];
        if (TRACE) T.kingShelter[colour][file == kingFile][file][distance]++;
    }    
}

void evaluatePassedPawns(EvalInfo* ei, Board* board, int colour){
    
    int sq, rank, canAdvance, safeAdvance;
    uint64_t tempPawns, destination, notEmpty;
    
    // Fetch Passed Pawns from the Pawn King Entry if we have one
    if (ei->pkentry != NULL) ei->passedPawns = ei->pkentry->passed;
    
    tempPawns = board->colours[colour] & ei->passedPawns;
    notEmpty  = board->colours[WHITE ] | board->colours[BLACK];
    
    // Evaluate each passed pawn
    while (tempPawns != 0ull){
        
        // Pop off the next passed Pawn
        sq = poplsb(&tempPawns);
        
        // Determine the releative rank
        rank = (colour == BLACK) ? (7 - Rank(sq)) : Rank(sq);
        
        // Determine where the pawn would advance to
        destination = (colour == BLACK) ? ((1ull << sq) >> 8): ((1ull << sq) << 8);
            
        // Destination does not have any pieces on it
        canAdvance = !(destination & notEmpty);
        
        // Destination is not attacked by the opponent
        safeAdvance = !(destination & ei->attacked[!colour]);
        
        ei->midgame[colour] += PassedPawn[canAdvance][safeAdvance][rank][MG];
        ei->endgame[colour] += PassedPawn[canAdvance][safeAdvance][rank][EG];
        if (TRACE) T.passedPawn[colour][canAdvance][safeAdvance][rank]++;
    }
}

void initializeEvalInfo(EvalInfo* ei, Board* board, PawnKingTable* pktable){
    
    uint64_t white   = board->colours[WHITE];
    uint64_t black   = board->colours[BLACK];
    uint64_t pawns   = board->pieces[PAWN];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK];
    uint64_t queens  = board->pieces[QUEEN];
    uint64_t kings   = board->pieces[KING];
    
    uint64_t whitePawns = white & pawns;
    uint64_t blackPawns = black & pawns;
    
    int wKingSq = getlsb((white & kings));
    int bKingSq = getlsb((black & kings));
    
    ei->pawnAttacks[WHITE] = ((whitePawns << 9) & ~FILE_A) | ((whitePawns << 7) & ~FILE_H);
    ei->pawnAttacks[BLACK] = ((blackPawns >> 9) & ~FILE_H) | ((blackPawns >> 7) & ~FILE_A);
    
    ei->rammedPawns[WHITE] = (blackPawns >> 8) & whitePawns;
    ei->rammedPawns[BLACK] = (whitePawns << 8) & blackPawns;
    
    ei->blockedPawns[WHITE] = ((whitePawns << 8) & (white | black)) >> 8;
    ei->blockedPawns[BLACK] = ((blackPawns >> 8) & (white | black)) << 8,
    
    ei->kingAreas[WHITE] = KingMap[wKingSq] | (1ull << wKingSq) | (KingMap[wKingSq] << 8);
    ei->kingAreas[BLACK] = KingMap[bKingSq] | (1ull << bKingSq) | (KingMap[bKingSq] >> 8);
    
    ei->mobilityAreas[WHITE] = ~(ei->pawnAttacks[BLACK] | (white & kings) | ei->blockedPawns[WHITE]);
    ei->mobilityAreas[BLACK] = ~(ei->pawnAttacks[WHITE] | (black & kings) | ei->blockedPawns[BLACK]);
    
    ei->attacked[WHITE] = kingAttacks(wKingSq, ~0ull);
    ei->attacked[BLACK] = kingAttacks(bKingSq, ~0ull);
    
    ei->occupiedMinusBishops[WHITE] = (white | black) ^ (white & (bishops | queens));
    ei->occupiedMinusBishops[BLACK] = (white | black) ^ (black & (bishops | queens));
    
    ei->occupiedMinusRooks[WHITE] = (white | black) ^ (white & (rooks | queens));
    ei->occupiedMinusRooks[BLACK] = (white | black) ^ (black & (rooks | queens));
    
    ei->passedPawns = 0ull;
    
    ei->attackCounts[WHITE] = ei->attackCounts[BLACK] = 0;
    ei->attackerCounts[WHITE] = ei->attackerCounts[BLACK] = 0;
    
    ei->midgame[WHITE] = ei->midgame[BLACK] = 0;
    ei->endgame[WHITE] = ei->endgame[BLACK] = 0;
    
    ei->pawnKingMidgame[WHITE] = ei->pawnKingMidgame[BLACK] = 0;
    ei->pawnKingEndgame[WHITE] = ei->pawnKingEndgame[BLACK] = 0;
    
    if (TEXEL) ei->pkentry = NULL;
    else       ei->pkentry = getPawnKingEntry(pktable, board->pkhash);
}

void initializeEvaluation(){
    
    int i;
    
    // Compute values for the King Safety based on the King Polynomial
    for (i = 0; i < 64; i++){
        KingSafety[i] = (int)(
            + KingPolynomial[0] * pow(i, 5) + KingPolynomial[1] * pow(i, 4)
            + KingPolynomial[2] * pow(i, 3) + KingPolynomial[3] * pow(i, 2)
            + KingPolynomial[4] * pow(i, 1) + KingPolynomial[5] * pow(i, 0)
        );
    }
}