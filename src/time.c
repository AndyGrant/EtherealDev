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

#include <stdlib.h>

#include "search.h"
#include "thread.h"
#include "time.h"
#include "types.h"
#include "uci.h"

int MoveOverhead = 250; // Set by UCI options

double getRealTime() {
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

double elapsedTime(SearchInfo *info) {
    return getRealTime() - info->startTime;
}

void initTimeManagment(SearchInfo *info, Limits *limits) {

    // Save off the start time
    info->startTime = limits->start;

    // Clear our stability time usage heuristics
    info->pvFactor = info->scoreFactor = 0;

    // Allocate time if Ethereal is handling the clock
    if (limits->limitedBySelf) {

        // Playing using X / Y + Z time control
        if (limits->mtg >= 0) {
            info->idealUsage =  0.75 * limits->time / (limits->mtg +  5) + limits->inc;
            info->maxAlloc   =  4.00 * limits->time / (limits->mtg +  7) + limits->inc;
            info->maxUsage   = 10.00 * limits->time / (limits->mtg + 10) + limits->inc;
        }

        // Playing using X + Y time controls
        else {
            info->idealUsage =  1.00 * (limits->time + 25 * limits->inc) / 50;
            info->maxAlloc   =  5.00 * (limits->time + 25 * limits->inc) / 50;
            info->maxUsage   = 10.00 * (limits->time + 25 * limits->inc) / 50;
        }

        // Cap time allocations using the move overhead
        info->idealUsage = MIN(info->idealUsage, limits->time - MoveOverhead);
        info->maxAlloc   = MIN(info->maxAlloc,   limits->time - MoveOverhead);
        info->maxUsage   = MIN(info->maxUsage,   limits->time - MoveOverhead);
    }

    // Interface told us to search for a predefined duration
    else if (limits->limitedByTime) {
        info->idealUsage = limits->timeLimit;
        info->maxAlloc   = limits->timeLimit;
        info->maxUsage   = limits->timeLimit;
    }
}

void updateTimeManagment(SearchInfo *info, Limits *limits) {

    const int thisValue = info->values[info->depth];
    const int lastValue = info->values[info->depth-1];
    const int scoreDiff = thisValue - lastValue;

    // Don't adjust time when we are at low depths, or if
    // we simply are not in control of our own time usage
    if (!limits->limitedBySelf || info->depth < 4) return;

    // Always scale the Score Factor torwards zero
    if (info->scoreFactor > 0) info->scoreFactor--;
    else if (info->scoreFactor < 0) info->scoreFactor++;

    // Adjust the Score Factor on score jumps and drops
    info->scoreFactor += BOUND(-ScoreFactorMax, ScoreFactorMax, scoreDiff / ScoreFactorDivisor);

    // Always scale back the PV Factor
    info->pvFactor = MAX(0, info->pvFactor - 1);

    // Reset the PV Factor if the best move changed
    if (info->bestMoves[info->depth] != info->bestMoves[info->depth-1])
        info->pvFactor = PVFactorCount;
}

int terminateTimeManagment(SearchInfo *info) {

    // Adjust our ideal usage based on variance in the best move and
    // scores between iterations of the search. We won't allow the new
    // usage value to exceed our maximum allocation. The cutoff is hit
    // elapsed time exceeds our initially computed ideal usage

    double cutoff = (info->idealUsage * 1.00)
                  + (info->idealUsage * info->pvFactor * PVFactorWeight)
                  + (info->idealUsage * info->scoreFactor * abs(ScoreFactorWeight));
    return elapsedTime(info) > MIN(cutoff, info->maxAlloc);
}

int terminateSearchEarly(Thread *thread) {

    // Terminate the search early if the max usage time has passed.
    // Only check this once for every 1024 nodes examined, in case
    // the system calls are quite slow. Always be sure to avoid an
    // early exit during a depth 1 search, to ensure a best move

    const Limits *limits = thread->limits;

    return  thread->depth > 1
        && (thread->nodes & 1023) == 1023
        && (limits->limitedBySelf || limits->limitedByTime)
        &&  elapsedTime(thread->info) >= thread->info->maxUsage;
}