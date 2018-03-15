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

#include <assert.h>
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

const int PawnValue[PHASE_NB]   = { 100, 121};

const int KnightValue[PHASE_NB] = { 458, 389};

const int BishopValue[PHASE_NB] = { 465, 411};

const int RookValue[PHASE_NB]   = { 634, 710};

const int QueenValue[PHASE_NB]  = {1275,1318};

const int KingValue[PHASE_NB]   = { 165, 165};

const int NoneValue[PHASE_NB]   = {   0,   0};

const int PawnIsolated[PHASE_NB] = {  -3,  -5};

const int PawnStacked[PHASE_NB] = { -11, -31};

const int PawnBackwards[2][PHASE_NB] = { {   6,  -3}, { -12, -11} };

const int PawnConnected32[32][PHASE_NB] = {
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
    {   0, -16}, {   6,   1}, {   2,  -3}, {   4,  19},
    {   7,   0}, {  21,   0}, {  14,   8}, {  17,  22},
    {   7,   0}, {  19,   4}, {  16,   8}, {  18,  17},
    {   7,  13}, {  20,  19}, {  19,  24}, {  38,  24},
    {  25,  55}, {  30,  64}, {  76,  61}, {  58,  76},
    { 134,  -5}, { 177,  16}, { 205,  24}, { 214,  84},
    {   0,   0}, {   0,   0}, {   0,   0}, {   0,   0},
};

const int KnightAttackedByPawn[PHASE_NB] = { -52, -33};

const int KnightOutpost[2][PHASE_NB] = { {  19, -34}, {  37,   8} };

const int KnightMobility[9][PHASE_NB] = {
    { -86, -98}, { -36, -85}, { -17, -40},
    {  -5, -11}, {   3, -13}, {   8,   0},
    {  18,  -1}, {  32,  -1}, {  48, -31},
};

const int BishopPair[PHASE_NB] = {  42,  67};

const int BishopAttackedByPawn[PHASE_NB] = { -55, -37};

const int BishopOutpost[2][PHASE_NB] = { {  19, -16}, {  51, -10} };

const int BishopMobility[14][PHASE_NB] = {
    { -61,-122}, { -47, -64}, { -18, -45}, {  -5, -21},
    {   4,  -7}, {  16,   0}, {  22,   7}, {  28,   3},
    {  29,  10}, {  35,   3}, {  40,   4}, {  50, -13},
    {  43,  -1}, {  38, -31},
};

const int RookFile[2][PHASE_NB] = { {  12,   1}, {  40,  -8} };

const int RookOnSeventh[PHASE_NB] = {   0,  24};

const int RookMobility[15][PHASE_NB] = {
    {-146, -94}, { -70,-119}, { -16, -66}, {  -8, -25},
    {  -7,  -3}, {  -7,  14}, {  -7,  25}, {  -3,  31},
    {   0,  36}, {   6,  36}, {  10,  42}, {  18,  47},
    {  19,  49}, {  24,  46}, {  15,  44},
};

const int QueenChecked[PHASE_NB] = { -40, -33};

const int QueenCheckedByPawn[PHASE_NB] = { -58, -47};

const int QueenMobility[28][PHASE_NB] = {
    { -60,-258}, {-247,-278}, { -42,-194}, { -36,-182},
    { -16,-120}, { -24, -61}, { -15, -88}, { -17, -84},
    { -13, -58}, {  -9, -52}, {  -8, -27}, {  -5, -28},
    {  -5, -15}, {  -2, -10}, {   0,  -7}, {   0,   4},
    {   1,  16}, {  -1,  14}, {   9,  24}, {  -2,  25},
    {   3,  28}, {  15,  28}, {  20,  12}, {  33,  15},
    {  47,  23}, {  51,   1}, { -12,  -2}, {  20,  10},
};

const int KingDefenders[12][PHASE_NB] = {
    { -37,  -4}, { -20,   5}, {   0,   0}, {   9,  -1},
    {  24,  -1}, {  35,   2}, {  39,   9}, {  29,-119},
    {  12,   6}, {  12,   6}, {  12,   6}, {  12,   6},
};

const int KingShelter[2][FILE_NB][RANK_NB][PHASE_NB] = {
  {{{ -15,  16}, {   6,  -8}, {  13,   2}, {  20,   4}, {   7,   2}, {  18,   1}, {  -1, -33}, { -31,   1}},
   {{   3,   7}, {  15,  -5}, {  14,  -8}, {   0, -11}, { -28,  -1}, { -68,  84}, {  63,  89}, { -32,   0}},
   {{  13,  13}, {   8,   0}, { -19,   0}, {  -9,   0}, { -28,  -3}, {   4, -13}, { -32,  63}, { -14,   1}},
   {{  12,  27}, {  15,   0}, {  -3,  -8}, {  19, -13}, {  16, -32}, { -27, -30}, { -74,  25}, {  -2,   0}},
   {{ -11,  18}, {   0,   1}, { -28,   1}, { -12,   2}, { -34, -14}, { -41, -23}, {  14,   0}, { -13,   0}},
   {{  23,   2}, {  17,  -4}, { -19,  -1}, {  -3, -20}, {   3, -32}, {  13, -48}, {  68, -37}, { -13,   0}},
   {{  20,   1}, {   3,  -9}, { -28,  -8}, { -20, -13}, { -25, -16}, { -37,   3}, {   0,  37}, { -28,   8}},
   {{ -14,  -3}, {  -1,  -8}, {   4,   0}, {   2,   3}, { -13,  11}, {   0,  30}, { -73,  71}, { -17,  15}}},
  {{{   0,   0}, {   1, -16}, {   5, -16}, { -56,  14}, {  14, -11}, {  -6,  40}, { -92,  -1}, { -55,  10}},
   {{   0,   0}, {  16,  -5}, {   6,  -5}, {  -3,  -5}, {   8, -26}, {  27,  89}, { -64,  19}, { -45,   3}},
   {{   0,   0}, {  23,   0}, {   0,  -5}, {  18, -25}, {  16,  -5}, { -18,  48}, { -33, -86}, { -21,  -1}},
   {{   0,   0}, {  -3,   8}, {  -8,  13}, { -14,   1}, { -27,  -2}, { -15,   2}, {  39, -35}, { -25,   0}},
   {{   0,   0}, {   4,   4}, {   7,  -6}, {  22,  -5}, {   8, -19}, { -51,  11}, { -36, -86}, {  -4,  -2}},
   {{   0,   0}, {  10,   1}, { -12,  -2}, { -16, -13}, {  14, -33}, { -21,   6}, {  21,  28}, { -28,   0}},
   {{   0,   0}, {  12,  -1}, {  -2,   0}, { -20,  -6}, { -23, -15}, {  12,   0}, { -96, -52}, { -44,  13}},
   {{   0,   0}, {   8, -27}, {   9, -15}, { -25,   0}, { -28,  -2}, {   2, -21}, {-212, -62}, { -44,  17}}},
};

const int PassedPawn[2][2][RANK_NB][PHASE_NB] = {
  {{{   0,   0}, { -30, -28}, { -23,   8}, { -13,  -2}, {  22,   0}, {  60,  -5}, { 152,  32}, {   0,   0}},
   {{   0,   0}, {  -2,   1}, { -13,  23}, { -12,  35}, {   7,  45}, {  70,  61}, { 192, 129}, {   0,   0}}},
  {{{   0,   0}, {  -7,  12}, { -11,   6}, {  -8,  27}, {  27,  32}, {  82,  63}, { 223, 149}, {   0,   0}},
   {{   0,   0}, {  -6,   8}, { -12,  17}, { -19,  52}, {  -9, 110}, {  36, 205}, { 126, 370}, {   0,   0}}},
};

const int KingSafety[256] = { // int(math.floor(800.0 / (1.0 + math.pow(math.e, -.047 * (x - 96)))))
      0,   0,   0,   0,   1,   1,   2,   2,   3,   3,   4,   4,   5,   6,   7,   8,
      8,   9,  10,  11,  12,  14,  15,  16,  17,  18,  20,  21,  22,  24,  25,  27,
     28,  30,  31,  33,  35,  36,  38,  40,  42,  43,  45,  47,  49,  51,  53,  55,
     57,  59,  62,  64,  66,  68,  71,  73,  75,  78,  80,  82,  85,  87,  90,  92,
     95,  98, 100, 103, 105, 108, 111, 114, 116, 119, 122, 125, 128, 131, 134, 137,
    140, 143, 146, 149, 152, 155, 158, 161, 164, 167, 171, 174, 177, 180, 184, 187,
    190, 194, 197, 200, 204, 207, 211, 214, 217, 221, 224, 228, 231, 235, 239, 242,
    246, 249, 253, 256, 260, 264, 267, 271, 275, 278, 282, 286, 290, 293, 297, 301,
    305, 308, 312, 316, 320, 324, 327, 331, 335, 339, 343, 347, 350, 354, 358, 362,
    366, 370, 374, 378, 381, 385, 389, 393, 397, 401, 405, 409, 413, 417, 421, 425,
    428, 432, 436, 440, 444, 448, 452, 456, 460, 464, 468, 471, 475, 479, 483, 487,
    491, 495, 499, 503, 506, 510, 514, 518, 522, 526, 529, 533, 537, 541, 545, 548,
    552, 556, 560, 563, 567, 571, 575, 578, 582, 586, 589, 593, 597, 600, 604, 607,
    611, 615, 618, 622, 625, 629, 632, 636, 639, 643, 646, 649, 653, 656, 660, 663,
    666, 670, 673, 676, 679, 683, 686, 689, 692, 695, 698, 700, 700, 700, 700, 700,
    700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700, 700,
};

const int KingThreatMissingPawns = 8;

const int KingThreatWeight[PIECE_NB] = {   4,   8,   8,  12,  16,   0};


const int Tempo[COLOUR_NB][PHASE_NB] = { {  25,  12}, { -25, -12} };


const int* PieceValues[8] = {
    PawnValue, KnightValue, BishopValue, RookValue,
    QueenValue, KingValue, NoneValue, NoneValue
};


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
    attacks = ei->pawnAttacks[colour] & ei->kingAreas[!colour];
    ei->attackedBy2[colour] = ei->attacked[colour] & ei->pawnAttacks[colour];
    ei->attacked[colour] |= ei->pawnAttacks[colour];
    ei->attackedNoQueen[colour] |= attacks;
    
    // Update the attack counts and attacker counts for pawns for use in
    // the king safety calculation. We just do this for the pawns as a whole,
    // and not individually, to save time, despite the loss in accuracy.
    if (attacks != 0ull){
        ei->attackCounts[colour] += KingThreatWeight[PAWN] * popcount(attacks);
        ei->attackerCounts[colour] += 1;
    }
    
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
    
    int sq, defended, mobilityCount;
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
        mobilityCount = popcount((ei->mobilityAreas[colour] & attacks));
        ei->midgame[colour] += KnightMobility[mobilityCount][MG];
        ei->endgame[colour] += KnightMobility[mobilityCount][EG];
        if (TRACE) T.knightMobility[colour][mobilityCount]++;
        
        // Update the attack and attacker counts for the
        // knight for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += KingThreatWeight[KNIGHT] * popcount(attacks);
            ei->attackerCounts[colour] += 1;
        }
    }
}

void evaluateBishops(EvalInfo* ei, Board* board, int colour){
    
    int sq, defended, mobilityCount;
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
        mobilityCount = popcount((ei->mobilityAreas[colour] & attacks));
        ei->midgame[colour] += BishopMobility[mobilityCount][MG];
        ei->endgame[colour] += BishopMobility[mobilityCount][EG];
        if (TRACE) T.bishopMobility[colour][mobilityCount]++;
        
        // Update the attack and attacker counts for the
        // bishop for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += KingThreatWeight[BISHOP] * popcount(attacks);
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
            ei->attackCounts[colour] += KingThreatWeight[ROOK] * popcount(attacks);
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
            ei->attackCounts[colour] += KingThreatWeight[QUEEN] * popcount(attacks);
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
        
        // Fetch the weight of the threats against our king
        attackCounts = ei->attackCounts[!colour];
        
        // Increse the threat if we are missing pawns around our king
        attackCounts += KingThreatMissingPawns * (3 - popcount(myPawns & ei->kingAreas[colour]));
        
        // Scale down the threat greatly if our opponent has no queen
        if (!(board->colours[!colour] & board->pieces[QUEEN]))
            attackCounts *= .25;
    
        ei->midgame[colour] -= KingSafety[MIN(255, MAX(0, attackCounts))];
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
    
    ei->blockedPawns[WHITE] = ((whitePawns << 8) & (white | black)) >> 8;
    ei->blockedPawns[BLACK] = ((blackPawns >> 8) & (white | black)) << 8,
    
    ei->kingAreas[WHITE] = KingMap[wKingSq] | (1ull << wKingSq) | (KingMap[wKingSq] << 8);
    ei->kingAreas[BLACK] = KingMap[bKingSq] | (1ull << bKingSq) | (KingMap[bKingSq] >> 8);
    
    ei->mobilityAreas[WHITE] = ~(ei->pawnAttacks[BLACK] | (white & kings) | ei->blockedPawns[WHITE]);
    ei->mobilityAreas[BLACK] = ~(ei->pawnAttacks[WHITE] | (black & kings) | ei->blockedPawns[BLACK]);
    
    ei->attacked[WHITE] = ei->attackedNoQueen[WHITE] = kingAttacks(wKingSq, ~0ull);
    ei->attacked[BLACK] = ei->attackedNoQueen[BLACK] = kingAttacks(bKingSq, ~0ull);
    
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
