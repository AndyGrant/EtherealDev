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

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#include <stdlib.h>

#include "search.h"
#include "time.h"
#include "types.h"
#include "uci.h"


int MoveOverhead = 100; // Set by UCI options


double getRealTime(){
#if defined(_WIN32) || defined(_WIN64)
    return (double)(GetTickCount());
#else
    struct timeval tv;
    double secsInMilli, usecsInMilli;

    gettimeofday(&tv, NULL);
    secsInMilli = ((double)tv.tv_sec) * 1000;
    usecsInMilli = tv.tv_usec / 1000;

    return secsInMilli + usecsInMilli;
#endif
}

double elapsedTime(SearchInfo* info){
    return getRealTime() - info->startTime;
}

void initTimeManagment(SearchInfo* info, Limits* limits){

    info->startTime = limits->start; // Save off the start time of the search

    info->pvFactor = info->scoreFactor = 0; // Clear time usage heuristics

    // Allocate time if Ethereal is handling the clock
    if (limits->limitedBySelf){

        // Playing using X / Y or X / Y + Z time controls
        if (limits->mtg >= 0){
            info->idealUsage =  0.75 * limits->time / (limits->mtg +  5) + limits->inc;
            info->maxAlloc   =  4.00 * limits->time / (limits->mtg +  7) + limits->inc;
            info->maxUsage   = 10.00 * limits->time / (limits->mtg + 10) + limits->inc;
        }

        // Playing using X + Y or X time controls
        else {
            info->idealUsage =  0.52 * (limits->time + 23 * limits->inc) / 25;
            info->maxAlloc   =  4.00 * (limits->time + 23 * limits->inc) / 25;
            info->maxUsage   = 10.00 * (limits->time + 23 * limits->inc) / 25;
        }

        // Cap all time allocations using the move time buffer
        info->idealUsage = MIN(info->idealUsage, limits->time - MoveOverhead);
        info->maxAlloc   = MIN(info->maxAlloc,   limits->time - MoveOverhead);
        info->maxUsage   = MIN(info->maxUsage,   limits->time - MoveOverhead);
    }

    // Interface told us to search for a predefined duration
    if (limits->limitedByTime){
        info->idealUsage = limits->timeLimit;
        info->maxAlloc   = limits->timeLimit;
        info->maxUsage   = limits->timeLimit;
    }
}

void updateTimeManagment(SearchInfo* info, Limits* limits, int depth, int value){

    const uint16_t thisMove = info->bestMoves[depth];
    const uint16_t lastMove = info->bestMoves[depth-1];
    const int lastValue     = info->values[depth-1];
    const int scoreDiff     = value - lastValue;

    // Don't adjust time when we are at low depths, or if
    // we simply are not in control of our own time usage
    if (!limits->limitedBySelf || depth < 10)
        return;

    // Always scale back the PV time factor
    info->pvFactor = MAX(-2, info->pvFactor - 1);

    // Increase time if the PV changed moves
    if (thisMove != lastMove)
        info->pvFactor = PVFactorCount;

    // Increase time when the score suddenly jumps
    info->scoreFactor += MAX(0, MIN(ScoreJumpMax,  scoreDiff / ScoreJumpDivisor));

    // Increase time when the score suddenly drops
    info->scoreFactor += MAX(0, MIN(ScoreDropMax, -scoreDiff / ScoreDropDivisor));
}

int terminateTimeManagment(SearchInfo* info) {

    double cutoff = info->idealUsage;

    // Adjust cutoff based on bestmove fluctuations
    cutoff *= 1.00 + info->pvFactor * PVFactorWeight;

    // Adjust cutoff bassed on score fluctuations
    cutoff *= 1.00 + info->scoreFactor * ScoreFactorWeight;

    // Terminate search if cutoff is reached
    return elapsedTime(info) > MIN(cutoff, info->maxAlloc);
}