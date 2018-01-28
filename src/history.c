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

#include <stdint.h>

#include "board.h"
#include "history.h"
#include "move.h"
#include "types.h"

enum { HISTORY_GOOD, HISTORY_TOTAL };

const uint64_t HistoryMax = 0x7FFFFFFF;

void reduceHistory(HistoryTable history){
    
    int c, f, t, i;
    
    for (c = 0; c < COLOUR_NB; c++)
        for (f = 0; f < SQUARE_NB; f++)
            for (t = 0; t < SQUARE_NB; t++)
                for (i = 0; i < 2; i++)
                    history[c][f][t][i] = 1 + history[c][f][t][i] / 4;
}

void updateHistory(HistoryTable history, uint16_t move, int colour, int isGood, int delta){
    
    int from = MoveFrom(move), to = MoveTo(move);
    
    // Update both counters by delta
    if (isGood) history[colour][from][to][HISTORY_GOOD] += delta;
    history[colour][from][to][HISTORY_TOTAL] += delta;
    
    // Divide the counters by two if we have exceeded the max value for history
    if (history[colour][from][to][HISTORY_TOTAL] >= HistoryMax){
        history[colour][from][to][HISTORY_GOOD] >>= 1;
        history[colour][from][to][HISTORY_TOTAL] >>= 1;
    }
}

int getHistoryScore(HistoryTable history, uint16_t move, int colour, int factor){
    
    int from  = MoveFrom(move);
    int to    = MoveTo(move);
    int good  = history[colour][from][to][HISTORY_GOOD ];
    int total = history[colour][from][to][HISTORY_TOTAL];
    return (factor * good) / total;
}

void reduceCounterMoveHistory(CounterMoveHistoryTable cmhistory){
    
    int c, p1, t1, p2, t2, i;
    
    for (c = 0; c < COLOUR_NB; c++)
        for (p1 = 0; p1 < PIECE_NB + 1; p1++)
            for (t1 = 0; t1 < SQUARE_NB; t1++)
                for (p2 = 0; p2 < PIECE_NB + 1; p2++)
                    for (t2 = 0; t2 < SQUARE_NB; t2++)
                        for (i = 0; i < 2; i++)
                            cmhistory[c][p1][t1][p2][t2][i] = 1 + cmhistory[c][p1][t1][p2][t2][i] / 4;
}

void updateCounterMoveHistory(CounterMoveHistoryTable cmhistory, Board* board, uint16_t move, int isGood, int delta){
    
    int c  = board->turn;
    int p1 = (MoveType(board->lastmove) == PROMOTION_MOVE) ? PAWN : PieceType(board->squares[MoveTo(board->lastmove)]);
    int t1 = MoveTo(board->lastmove);
    int p2 = PieceType(board->squares[MoveFrom(move)]);
    int t2 = MoveTo(move);
    
    if (isGood) cmhistory[c][p1][t1][p2][t2][HISTORY_GOOD]  += delta;
    cmhistory[c][p1][t1][p2][t2][HISTORY_TOTAL] += delta;
    
    if (cmhistory[c][p1][t1][p2][t2][HISTORY_TOTAL] >= HistoryMax){
        cmhistory[c][p1][t1][p2][t2][HISTORY_GOOD] >>= 1;
        cmhistory[c][p1][t1][p2][t2][HISTORY_TOTAL] >>= 1;
    }
}

int getCounterMoveHistoryScore(CounterMoveHistoryTable cmhistory, Board* board, uint16_t move, int factor){
    
    int c  = board->turn;
    int p1 = PieceType(board->squares[MoveTo(board->lastmove)]);
    int t1 = MoveTo(board->lastmove);
    int p2 = PieceType(board->squares[MoveFrom(move)]);
    int t2 = MoveTo(move);
    
    int good  = cmhistory[c][p1][t1][p2][t2][HISTORY_GOOD ];
    int total = cmhistory[c][p1][t1][p2][t2][HISTORY_TOTAL];
    
    return (factor * good) / total;
}
