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

#include "attacks.h"
#include "board.h"
#include "bitboards.h"
#include "castle.h"
#include "evaluate.h"
#include "masks.h"
#include "movegen.h"
#include "piece.h"
#include "psqt.h"
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

// Undefined after all evaluation terms
#define S(mg, eg) (MakeScore((mg), (eg)))


// Definition of Values for each Piece type

const int PawnValue   = S( 100, 115);
const int KnightValue = S( 429, 364);
const int BishopValue = S( 441, 385);
const int RookValue   = S( 600, 666);
const int QueenValue  = S(1225,1250);
const int KingValue   = S(   0,   0);

const int PieceValues[8][PHASE_NB] = {
    { 100, 115}, { 429, 364}, { 441, 385}, { 600, 666},
    {1225,1250}, {   0,   0}, {   0,   0}, {   0,   0},
};

// Definition of evaluation terms related to Pawns

const int PawnIsolated = S(  -2,  -7);

const int PawnStacked = S( -12, -31);

const int PawnBackwards[2] = { S(   4,  -3), S( -10, -11) };

const int PawnConnected32[32] = {
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
    S(   0, -11), S(   6,   4), S(   1,   0), S(   3,  19),
    S(   9,   2), S(  21,   2), S(  14,   9), S(  15,  21),
    S(   5,   0), S(  21,   5), S(  15,   8), S(  17,  18),
    S(   8,   9), S(  21,  19), S(  24,  23), S(  39,  26),
    S(  21,  35), S(  31,  48), S(  70,  54), S(  62,  72),
    S(  89, -56), S( 177, -26), S( 201,  -7), S( 238,  39),
    S(   0,   0), S(   0,   0), S(   0,   0), S(   0,   0),
};


// Definition of evaluation terms related to Knights

const int KnightRammedPawns = S(  -1,   6);

const int KnightOutpost[2] = { S(  22, -31), S(  36,   6) };

const int KnightMobility[9] = {
    S( -77, -85), S( -34, -87), S( -19, -42),
    S(  -6, -17), S(   0, -14), S(   5,  -2),
    S(  16,  -1), S(  29,  -2), S(  45, -31),
};


// Definition of evaluation terms related to Bishops

const int BishopPair = S(  35,  63);

const int BishopRammedPawns = S( -11,  -8);

const int BishopOutpost[2] = { S(  21, -16), S(  50,  -6) };

const int BishopMobility[14] = {
    S( -67,-119), S( -44, -70), S( -18, -45), S(  -7, -22),
    S(   4, -10), S(  16,   0), S(  21,   6), S(  26,   3),
    S(  27,   8), S(  34,   2), S(  36,   1), S(  42, -18),
    S(  43, -12), S(  19, -51),
};


// Definition of evaluation terms related to Rooks

const int RookFile[2] = { S(  12,  -1), S(  39,  -8) };

const int RookOnSeventh = S(   8,  26);

const int RookMobility[15] = {
    S(-153,-136), S( -59,-120), S( -14, -66), S(  -8, -26),
    S(  -7,  -5), S(  -8,  10), S(  -8,  21), S(  -4,  27),
    S(   1,  32), S(   7,  34), S(  11,  40), S(  19,  46),
    S(  21,  47), S(  26,  43), S(  19,  37),
};


// Definition of evaluation terms related to Queens

const int QueenMobility[28] = {
    S(-153,-318), S(-207,-367), S( -71,-243), S( -32,-182),
    S( -18,-130), S( -22, -68), S( -15, -89), S( -15, -79),
    S( -13, -58), S( -10, -52), S(  -8, -27), S(  -5, -26),
    S(  -3, -12), S(   1,  -8), S(   2,  -3), S(   1,   6),
    S(   4,  16), S(   1,  16), S(   8,  21), S(   0,  23),
    S(   0,  20), S(  11,  18), S(   9,   2), S(  24,  -1),
    S(  28,   3), S(  40, -15), S( -43, -20), S( -18, -23),
};


// Definition of evaluation terms related to Kings

const int KingDefenders[12] = {
    S( -30,   4), S( -18,   0), S(   0,   0), S(  10,   0),
    S(  21,  -3), S(  32,   0), S(  38,   6), S(  15,-231),
    S(  11,   5), S(  11,   5), S(  11,   5), S(  11,   5),
};

const int KingThreatWeight[PIECE_NB] = { 4, 8, 8, 12, 16, 0 };

int KingSafety[256]; // Defined by the Polynomial below

const double KingPolynomial[6] = {
    0.00000011, -0.00009948,  0.00797308,
    0.03141319,  2.18429452, -3.33669140,
};

const int KingShelter[2][FILE_NB][RANK_NB] = {
  {{S( -18,   4), S(   6, -17), S(  16,  -1), S(  22,   0), S(   8,   1), S(  16,  -6), S(  31, -25), S( -22,  13)},
   {S(   1,   5), S(  17, -10), S(  12,  -8), S(  -4,  -9), S( -20,   0), S( -57,  48), S(  86,  63), S( -27,   4)},
   {S(  10,  11), S(   9,  -2), S( -19,   0), S( -12,  -2), S( -28,  -8), S(  -3, -18), S(  -6,  54), S( -14,   1)},
   {S(   8,  26), S(  10,  -2), S(  -4,  -9), S(  21, -14), S(  18, -29), S( -12, -31), S(-134,  32), S(  -2,  -2)},
   {S( -16,  18), S(  -2,   1), S( -28,   2), S(  -8,   4), S( -21, -15), S( -23, -13), S(  30,  -6), S( -15,  -2)},
   {S(  18,   0), S(  17,  -2), S( -20,   0), S(  -2, -20), S(  10, -28), S(  27, -41), S(  33, -41), S( -12,   0)},
   {S(  13,   0), S(   0, -10), S( -28,  -7), S( -14, -14), S( -22, -22), S( -47,   4), S( -37,  33), S( -25,   9)},
   {S( -18,  -4), S(  -1, -10), S(   6,   0), S(   4,   3), S(  -9,  12), S(   5,  31), S(-170,  75), S( -15,  18)}},
  {{S(   0,   0), S(  -6, -22), S(   0, -17), S( -45,  10), S(  -7,  -8), S(   4,  49), S(-106,  -5), S( -37,  28)},
   {S(   0,   0), S(  16,  -6), S(  10,  -4), S(  -6,  -6), S(  14, -25), S(  26,  71), S(-163, -19), S( -37,   8)},
   {S(   0,   0), S(  27,   0), S(  -4,  -7), S(  19, -21), S(  16,   0), S(-105,  39), S( -41, -40), S( -21,  -1)},
   {S(   0,   0), S(  -5,   8), S( -11,  10), S( -17,  -1), S( -29,  -3), S( -98,   2), S( -24, -57), S( -29,  -2)},
   {S(   0,   0), S(   7,   3), S(   5,  -5), S(  23,  -4), S(  14, -19), S( -49,   2), S(-149, -73), S(  -7,  -6)},
   {S(   0,   0), S(   9,   0), S( -14,   0), S( -10, -15), S(  28, -37), S( -37,   0), S(  20,  35), S( -29,   0)},
   {S(   0,   0), S(  11,   0), S(   0,   1), S( -16,  -2), S( -13,  -8), S(  19,  12), S( -72, -49), S( -41,  13)},
   {S(   0,   0), S(   3, -28), S(   5, -17), S( -27,  -5), S( -31,  -1), S(  -2, -12), S(-199, -35), S( -40,  24)}},
};


// Definition of evaluation terms related to Passed Pawns

const int PassedPawn[2][2][RANK_NB] = {
  {{S(   0,   0), S( -23, -18), S( -27,   5), S( -12,  -1), S(  23,   0), S(  59,  -7), S( 139,  33), S(   0,   0)},
   {S(   0,   0), S(   0,   4), S( -15,  23), S( -10,  34), S(   9,  41), S(  61,  48), S( 176, 101), S(   0,   0)}},
  {{S(   0,   0), S(  -6,  13), S( -10,  10), S(  -2,  26), S(  31,  31), S(  77,  48), S( 223, 120), S(   0,   0)},
   {S(   0,   0), S(  -2,  13), S(  -8,  20), S( -13,  54), S(   0, 108), S(  44, 198), S( 111, 344), S(   0,   0)}},
};


// Definition of evaluation terms related to Threats

const int ThreatPawnAttackedByOne    = S( -20, -19);

const int ThreatMinorAttackedByPawn  = S( -64, -46);

const int ThreatMinorAttackedByMajor = S( -36, -28);

const int ThreatQueenAttackedByMinor = S( -30, -16);

const int ThreatQueenAttackedByOne   = S( -40, -21);


// Definition of evaluation terms related to general properties

const int Tempo[COLOUR_NB] = { S(  25,  12), S( -25, -12) };


#undef S // Undefine MakeScore


int evaluateBoard(Board* board, PawnKingTable* pktable){

    EvalInfo ei; // Fix bogus GCC warning
    ei.pkeval[WHITE] = ei.pkeval[BLACK] = 0;

    int phase, eval, pkeval;

    // Check for obviously drawn positions
    if (evaluateDraws(board)) return 0;

    // Setup and perform the evaluation of all pieces
    initializeEvalInfo(&ei, board, pktable);
    eval = evaluatePieces(&ei, board);

    // Store a new Pawn King Entry if we did not have one
    if (ei.pkentry == NULL && !TEXEL){
        pkeval = ei.pkeval[WHITE] - ei.pkeval[BLACK];
        storePawnKingEntry(pktable, board->pkhash, ei.passedPawns, pkeval);
    }

    // Add in the PSQT and Material values, as well as the tempo
    eval += board->psqtmat + Tempo[board->turn];

    // Calcuate the game phase based on remaining material (Fruit Method)
    phase = 24 - popcount(board->pieces[QUEEN]) * 4
               - popcount(board->pieces[ROOK]) * 2
               - popcount(board->pieces[KNIGHT] | board->pieces[BISHOP]);
    phase = (phase * 256 + 12) / 24;

    // Compute the interpolated evaluation
    eval  = (ScoreMG(eval) * (256 - phase) + ScoreEG(eval) * phase) / 256;

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

int evaluatePieces(EvalInfo* ei, Board* board){

    int eval = 0;

    eval += evaluatePawns(ei, board, WHITE)
          - evaluatePawns(ei, board, BLACK);

    eval += evaluateKnights(ei, board, WHITE)
          - evaluateKnights(ei, board, BLACK);

    eval += evaluateBishops(ei, board, WHITE)
          - evaluateBishops(ei, board, BLACK);

    eval += evaluateRooks(ei, board, WHITE)
          - evaluateRooks(ei, board, BLACK);

    eval += evaluateQueens(ei, board, WHITE)
          - evaluateQueens(ei, board, BLACK);

    eval += evaluateKings(ei, board, WHITE)
          - evaluateKings(ei, board, BLACK);

    eval += evaluatePassedPawns(ei, board, WHITE)
          - evaluatePassedPawns(ei, board, BLACK);

    eval += evaluateThreats(ei, board, WHITE)
          - evaluateThreats(ei, board, BLACK);

    return eval;
}

int evaluatePawns(EvalInfo* ei, Board* board, int colour){

    const int forward = (colour == WHITE) ? 8 : -8;

    int sq, semi, eval = 0;
    uint64_t pawns, myPawns, tempPawns, enemyPawns, attacks;

    // Update the attacks array with the pawn attacks. We will use this to
    // determine whether or not passed pawns may advance safely later on.
    // It is also used to compute pawn threats against minors and majors
    ei->attackedBy2[colour]      = ei->pawnAttacks[colour] & ei->attacked[colour];
    ei->attacked[colour]        |= ei->pawnAttacks[colour];
    ei->attackedBy[colour][PAWN] = ei->pawnAttacks[colour];

    // Update the attack counts for our pawns. We will not count squares twice
    // even if they are attacked by two pawns. Also, we do not count pawns
    // torwards our attackers counts, which is used to decide when to look
    // at the King Safety of a position.
    attacks = ei->pawnAttacks[colour] & ei->kingAreas[!colour];
    ei->attackCounts[colour] += KingThreatWeight[PAWN] * popcount(attacks);

    // The pawn table holds the rest of the eval information we will calculate.
    // We return the saved value only when we evaluate for white, since we save
    // the evaluation as a combination of white and black, from white's POV
    if (ei->pkentry != NULL) return colour == WHITE ? ei->pkentry->eval : 0;

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
        if (!(passedPawnMasks(colour, sq) & enemyPawns))
            ei->passedPawns |= (1ull << sq);

        // Apply a penalty if the pawn is isolated
        if (!(isolatedPawnMasks(sq) & tempPawns)){
            eval += PawnIsolated;
            if (TRACE) T.pawnIsolated[colour]++;
        }

        // Apply a penalty if the pawn is stacked
        if (Files[fileOf(sq)] & tempPawns){
            eval += PawnStacked;
            if (TRACE) T.pawnStacked[colour]++;
        }

        // Apply a penalty if the pawn is backward
        if (   !(passedPawnMasks(!colour, sq) & myPawns)
            &&  (ei->pawnAttacks[!colour] & (1ull << (sq + forward)))){
            semi = !(Files[fileOf(sq)] & enemyPawns);
            eval += PawnBackwards[semi];
            if (TRACE) T.pawnBackwards[colour][semi]++;
        }

        // Apply a bonus if the pawn is connected and not backward
        else if (pawnConnectedMasks(colour, sq) & myPawns){
            eval += PawnConnected32[relativeSquare32(sq, colour)];
            if (TRACE) T.pawnConnected[colour][sq]++;
        }
    }

    ei->pkeval[colour] = eval;

    return eval;
}

int evaluateKnights(EvalInfo* ei, Board* board, int colour){

    int sq, defended, count, eval = 0;
    uint64_t tempKnights, enemyPawns, attacks;

    tempKnights = board->pieces[KNIGHT] & board->colours[colour];
    enemyPawns = board->pieces[PAWN] & board->colours[!colour];

    ei->attackedBy[colour][KNIGHT] = 0ull;

    // Evaluate each knight
    while (tempKnights){

        // Pop off the next knight
        sq = poplsb(&tempKnights);

        if (TRACE) T.knightCounts[colour]++;
        if (TRACE) T.knightPSQT[colour][sq]++;

        // Update the attacks array with the knight attacks. We will use this to
        // determine whether or not passed pawns may advance safely later on.
        attacks = knightAttacks(sq);
        ei->attackedBy2[colour]        |= attacks & ei->attacked[colour];
        ei->attacked[colour]           |= attacks;
        ei->attackedBy[colour][KNIGHT] |= attacks;

        // Apply a bonus for the knight based on number of rammed pawns
        count = popcount(ei->rammedPawns[colour]);
        eval += count * KnightRammedPawns;
        if (TRACE) T.knightRammedPawns[colour] += count;

        // Apply a bonus if the knight is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the knight.
        if (    (outpostRanks(colour) & (1ull << sq))
            && !(outpostSquareMasks(colour, sq) & enemyPawns)){
            defended = !!(ei->pawnAttacks[colour] & (1ull << sq));
            eval += KnightOutpost[defended];
            if (TRACE) T.knightOutpost[colour][defended]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the knight
        count = popcount((ei->mobilityAreas[colour] & attacks));
        eval += KnightMobility[count];
        if (TRACE) T.knightMobility[colour][count]++;

        // Update the attack and attacker counts for the
        // knight for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += KingThreatWeight[KNIGHT] * popcount(attacks);
            ei->attackerCounts[colour] += 1;
        }
    }

    return eval;
}

int evaluateBishops(EvalInfo* ei, Board* board, int colour){

    int sq, defended, count, eval = 0;
    uint64_t tempBishops, enemyPawns, attacks;

    tempBishops = board->pieces[BISHOP] & board->colours[colour];
    enemyPawns = board->pieces[PAWN] & board->colours[!colour];

    ei->attackedBy[colour][BISHOP] = 0ull;

    // Apply a bonus for having a pair of bishops
    if ((tempBishops & WHITE_SQUARES) && (tempBishops & BLACK_SQUARES)){
        eval += BishopPair;
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
        attacks = bishopAttacks(sq, ei->occupiedMinusBishops[colour]);
        ei->attackedBy2[colour]        |= attacks & ei->attacked[colour];
        ei->attacked[colour]           |= attacks;
        ei->attackedBy[colour][BISHOP] |= attacks;

        // Apply a penalty for the bishop based on number of rammed pawns
        // of our own colour, which reside on the same shade of square as the bishop
        count = popcount(ei->rammedPawns[colour] & (((1ull << sq) & WHITE_SQUARES ? WHITE_SQUARES : BLACK_SQUARES)));
        eval += count * BishopRammedPawns;
        if (TRACE) T.bishopRammedPawns[colour] += count;

        // Apply a bonus if the bishop is on an outpost square, and cannot be attacked
        // by an enemy pawn. Increase the bonus if one of our pawns supports the bishop.
        if (    (outpostRanks(colour) & (1ull << sq))
            && !(outpostSquareMasks(colour, sq) & enemyPawns)){
            defended = !!(ei->pawnAttacks[colour] & (1ull << sq));
            eval += BishopOutpost[defended];
            if (TRACE) T.bishopOutpost[colour][defended]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the bishop
        count = popcount((ei->mobilityAreas[colour] & attacks));
        eval += BishopMobility[count];
        if (TRACE) T.bishopMobility[colour][count]++;

        // Update the attack and attacker counts for the
        // bishop for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += KingThreatWeight[BISHOP] * popcount(attacks);
            ei->attackerCounts[colour] += 1;
        }
    }

    return eval;
}

int evaluateRooks(EvalInfo* ei, Board* board, int colour){

    int sq, open, count, eval = 0;
    uint64_t attacks;

    uint64_t myPawns    = board->pieces[PAWN] & board->colours[ colour];
    uint64_t enemyPawns = board->pieces[PAWN] & board->colours[!colour];
    uint64_t tempRooks  = board->pieces[ROOK] & board->colours[ colour];
    uint64_t enemyKings = board->pieces[KING] & board->colours[!colour];

    ei->attackedBy[colour][ROOK] = 0ull;

    // Evaluate each rook
    while (tempRooks){

        // Pop off the next rook
        sq = poplsb(&tempRooks);

        if (TRACE) T.rookCounts[colour]++;
        if (TRACE) T.rookPSQT[colour][sq]++;

        // Update the attacks array with the rooks attacks. We will use this to
        // determine whether or not passed pawns may advance safely later on.
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[colour]);
        ei->attackedBy2[colour]      |= attacks & ei->attacked[colour];
        ei->attacked[colour]         |= attacks;
        ei->attackedBy[colour][ROOK] |= attacks;

        // Rook is on a semi-open file if there are no
        // pawns of the Rook's colour on the file. If
        // there are no pawns at all, it is an open file
        if (!(myPawns & Files[fileOf(sq)])){
            open = !(enemyPawns & Files[fileOf(sq)]);
            eval += RookFile[open];
            if (TRACE) T.rookFile[colour][open]++;
        }

        // Rook gains a bonus for being located on seventh rank relative to its
        // colour so long as the enemy king is on the last two ranks of the board
        if (   rankOf(sq) == (colour == BLACK ? 1 : 6)
            && relativeRankOf(colour, getlsb(enemyKings)) >= 6) {
            eval += RookOnSeventh;
            if (TRACE) T.rookOnSeventh[colour]++;
        }

        // Apply a bonus (or penalty) based on the mobility of the rook
        count = popcount((ei->mobilityAreas[colour] & attacks));
        eval += RookMobility[count];
        if (TRACE) T.rookMobility[colour][count]++;

        // Update the attack and attacker counts for the
        // rook for use in the king safety calculation.
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += KingThreatWeight[ROOK] * popcount(attacks);
            ei->attackerCounts[colour] += 1;
        }
    }

    return eval;
}

int evaluateQueens(EvalInfo* ei, Board* board, int colour){

    int sq, count, eval = 0;
    uint64_t tempQueens, attacks;

    tempQueens = board->pieces[QUEEN] & board->colours[colour];

    ei->attackedBy[colour][QUEEN] = 0ull;

    // Evaluate each queen
    while (tempQueens){

        // Pop off the next queen
        sq = poplsb(&tempQueens);

        if (TRACE) T.queenCounts[colour]++;
        if (TRACE) T.queenPSQT[colour][sq]++;

        // Update the attacks array with the rooks attacks. We will use this to
        // determine whether or not passed pawns may advance safely later on.
        attacks = rookAttacks(sq, ei->occupiedMinusRooks[colour])
                | bishopAttacks(sq, ei->occupiedMinusBishops[colour]);
        ei->attackedBy2[colour]       |= attacks & ei->attacked[colour];
        ei->attacked[colour]          |= attacks;
        ei->attackedBy[colour][QUEEN] |= attacks;

        // Apply a bonus (or penalty) based on the mobility of the queen
        count = popcount((ei->mobilityAreas[colour] & attacks));
        eval += QueenMobility[count];
        if (TRACE) T.queenMobility[colour][count]++;

        // Update the attack and attacker counts for the queen for use in
        // the king safety calculation. We could the Queen as two attacking
        // pieces. This way King Safety is always used with the Queen attacks
        attacks = attacks & ei->kingAreas[!colour];
        if (attacks != 0ull){
            ei->attackCounts[colour] += KingThreatWeight[QUEEN] * popcount(attacks);
            ei->attackerCounts[colour] += 2;
        }
    }

    return eval;
}

int evaluateKings(EvalInfo* ei, Board* board, int colour){

    int file, kingFile, kingRank, kingSq;
    int distance, count, eval = 0, pkeval = 0;

    uint64_t filePawns, weak;

    uint64_t myPawns = board->pieces[PAWN] & board->colours[colour];
    uint64_t myKings = board->pieces[KING] & board->colours[colour];

    uint64_t myDefenders  = (board->pieces[PAWN  ] & board->colours[colour])
                          | (board->pieces[KNIGHT] & board->colours[colour])
                          | (board->pieces[BISHOP] & board->colours[colour]);

    kingSq = getlsb(myKings);
    kingFile = fileOf(kingSq);
    kingRank = rankOf(kingSq);

    // For Tuning Piece Square Table for Kings
    if (TRACE) T.kingPSQT[colour][kingSq]++;

    // Bonus for our pawns and minors sitting within our king area
    count = popcount(myDefenders & ei->kingAreas[colour]);
    eval += KingDefenders[count];
    if (TRACE) T.kingDefenders[colour][count]++;

    // If we have two or more threats to our king area, we will apply a penalty
    // based on the number of squares attacked, and the strength of the attackers
    if (ei->attackerCounts[!colour] >= 2){

        // Attacked squares are weak if we only defend once with king or queen.
        // This definition of square weakness is taken from Stockfish's king safety
        weak =  ei->attacked[!colour]
             & ~ei->attackedBy2[colour]
             & (  ~ei->attacked[colour]
                |  ei->attackedBy[colour][QUEEN]
                |  ei->attackedBy[colour][KING]);

        // Compute King Safety index based on safety factors
        count =  8                                              // King Safety Baseline
              +  1 * ei->attackCounts[!colour]                  // Computed attack weights
              + 16 * popcount(weak & ei->kingAreas[colour])     // Weak squares in King Area
              -  8 * popcount(myPawns & ei->kingAreas[colour]); // Pawns sitting in our King Area

        // Scale down attack count if there are no enemy queens
        if (!(board->colours[!colour] & board->pieces[QUEEN]))
            count *= .25;

        eval -= KingSafety[MIN(255, MAX(0, count))];
    }

    // Pawn Shelter evaluation is stored in the PawnKing evaluation table
    if (ei->pkentry != NULL) return eval;

    // Evaluate Pawn Shelter. We will look at the King's file and any adjacent files
    // to the King's file. We evaluate the distance between the king and the most backward
    // pawn. We will not look at pawns behind the king, and will consider that as having
    // no pawn on the file. No pawn on a file is used with distance equals 7, as no pawn
    // can ever be a distance of 7 from the king. Different bonus is in order when we are
    // looking at the file on which the King sits.

    for (file = MAX(0, kingFile - 1); file <= MIN(7, kingFile + 1); file++){

        filePawns = myPawns & Files[file] & ranksAtOrAboveMasks(colour, kingRank);

        distance = filePawns ?
                   colour == WHITE ? rankOf(getlsb(filePawns)) - kingRank
                                   : kingRank - rankOf(getmsb(filePawns))
                                   : 7;

        pkeval += KingShelter[file == kingFile][file][distance];
        if (TRACE) T.kingShelter[colour][file == kingFile][file][distance]++;
    }

    ei->pkeval[colour] += pkeval;

    return eval + pkeval;
}

int evaluatePassedPawns(EvalInfo* ei, Board* board, int colour){

    int sq, rank, canAdvance, safeAdvance, eval = 0;
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
        rank = (colour == BLACK) ? (7 - rankOf(sq)) : rankOf(sq);

        // Determine where the pawn would advance to
        destination = (colour == BLACK) ? ((1ull << sq) >> 8): ((1ull << sq) << 8);

        // Destination does not have any pieces on it
        canAdvance = !(destination & notEmpty);

        // Destination is not attacked by the opponent
        safeAdvance = !(destination & ei->attacked[!colour]);

        eval += PassedPawn[canAdvance][safeAdvance][rank];
        if (TRACE) T.passedPawn[colour][canAdvance][safeAdvance][rank]++;
    }

    return eval;
}

int evaluateThreats(EvalInfo* ei, Board* board, int colour){

    int count, eval = 0;

    uint64_t pawns   = board->colours[colour] & board->pieces[PAWN  ];
    uint64_t knights = board->colours[colour] & board->pieces[KNIGHT];
    uint64_t bishops = board->colours[colour] & board->pieces[BISHOP];
    uint64_t queens  = board->colours[colour] & board->pieces[QUEEN ];

    uint64_t attacksByPawns  = ei->attackedBy[!colour][PAWN  ];
    uint64_t attacksByMinors = ei->attackedBy[!colour][KNIGHT] | ei->attackedBy[!colour][BISHOP];
    uint64_t attacksByMajors = ei->attackedBy[!colour][ROOK  ] | ei->attackedBy[!colour][QUEEN ];

    // Penalty for each unsupported pawn on the board
    count = popcount(pawns & ~ei->attacked[colour] & ei->attacked[!colour]);
    eval += count * ThreatPawnAttackedByOne;
    if (TRACE) T.threatPawnAttackedByOne[colour] += count;

    // Penalty for pawn threats against our minors
    count = popcount((knights | bishops) & attacksByPawns);
    eval += count * ThreatMinorAttackedByPawn;
    if (TRACE) T.threatMinorAttackedByPawn[colour] += count;

    // Penalty for all major threats against our unsupported knights and bishops
    count = popcount((knights | bishops) & ~ei->attacked[colour] & attacksByMajors);
    eval += count * ThreatMinorAttackedByMajor;
    if (TRACE) T.threatMinorAttackedByMajor[colour] += count;

    // Penalty for all minor threats against our queens
    count = popcount(queens & (attacksByMinors));
    eval += count * ThreatQueenAttackedByMinor;
    if (TRACE) T.threatQueenAttackedByMinor[colour] += count;

    // Penalty for any threat against our queens
    count = popcount(queens & ei->attacked[!colour]);
    eval += count * ThreatQueenAttackedByOne;
    if (TRACE) T.threatQueenAttackedByOne[colour] += count;

    return eval;
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

    ei->kingAreas[WHITE] = kingAttacks(wKingSq) | (1ull << wKingSq) | (kingAttacks(wKingSq) << 8);
    ei->kingAreas[BLACK] = kingAttacks(bKingSq) | (1ull << bKingSq) | (kingAttacks(bKingSq) >> 8);

    ei->mobilityAreas[WHITE] = ~(ei->pawnAttacks[BLACK] | (white & kings) | ei->blockedPawns[WHITE]);
    ei->mobilityAreas[BLACK] = ~(ei->pawnAttacks[WHITE] | (black & kings) | ei->blockedPawns[BLACK]);

    ei->attacked[WHITE] = ei->attackedBy[WHITE][KING] = kingAttacks(wKingSq);
    ei->attacked[BLACK] = ei->attackedBy[BLACK][KING] = kingAttacks(bKingSq);

    ei->occupiedMinusBishops[WHITE] = (white | black) ^ (white & (bishops | queens));
    ei->occupiedMinusBishops[BLACK] = (white | black) ^ (black & (bishops | queens));

    ei->occupiedMinusRooks[WHITE] = (white | black) ^ (white & (rooks | queens));
    ei->occupiedMinusRooks[BLACK] = (white | black) ^ (black & (rooks | queens));

    ei->passedPawns = 0ull;

    ei->attackCounts[WHITE] = ei->attackCounts[BLACK] = 0;
    ei->attackerCounts[WHITE] = ei->attackerCounts[BLACK] = 0;

    if (TEXEL) ei->pkentry = NULL;
    else       ei->pkentry = getPawnKingEntry(pktable, board->pkhash);
}

void initializeEvaluation(){

    int i;

    // Compute values for the King Safety based on the King Polynomial

    for (i = 0; i < 256; i++){

        KingSafety[i] = (int)(
            + KingPolynomial[0] * pow(i / 4.0, 5)
            + KingPolynomial[1] * pow(i / 4.0, 4)
            + KingPolynomial[2] * pow(i / 4.0, 3)
            + KingPolynomial[3] * pow(i / 4.0, 2)
            + KingPolynomial[4] * pow(i / 4.0, 1)
            + KingPolynomial[5] * pow(i / 4.0, 0)
        );

        KingSafety[i] = MakeScore(KingSafety[i], 0);
    }
}
