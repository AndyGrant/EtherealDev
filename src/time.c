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

#include "search.h"
#include "thread.h"
#include "time.h"
#include "types.h"
#include "uci.h"

int MoveOverhead = 300; // Set by UCI options

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

double elapsedTime(const SearchInfo *info) {
    return getRealTime() - info->startTime;
}

void initTimeManagment(const Limits *limits, SearchInfo *info) {

    info->startTime = limits->start; // Save off the start time of the search

    info->pv_stability = 0; // Clear our stability time usage heuristic

    memset(info->nodes, 0, sizeof(uint16_t) * 0x10000); // Clear Node counters

    // Allocate time if Ethereal is handling the clock
    if (limits->limitedBySelf) {

        // Playing using X / Y + Z time control
        if (limits->mtg >= 0) {
            info->idealUsage =  2.50 * (limits->time - MoveOverhead) / (limits->mtg +  5) + limits->inc;
            info->maxUsage   = 10.00 * (limits->time - MoveOverhead) / (limits->mtg + 10) + limits->inc;
        }

        // Playing using X + Y time controls
        else {
            info->idealUsage =  2.50 * ((limits->time - MoveOverhead) + 25 * limits->inc) / 50;
            info->maxUsage   = 10.00 * ((limits->time - MoveOverhead) + 25 * limits->inc) / 50;
        }

        // Cap time allocations using the move overhead
        info->idealUsage = MIN(info->idealUsage, limits->time - MoveOverhead);
        info->maxUsage   = MIN(info->maxUsage,   limits->time - MoveOverhead);
    }

    // Interface told us to search for a predefined duration
    if (limits->limitedByTime) {
        info->idealUsage = limits->timeLimit;
        info->maxUsage   = limits->timeLimit;
    }
}

void update_time_manager(const Thread *thread, const Limits *limits, SearchInfo *info) {

    // Don't update the self-clock at low depths
    if (!limits->limitedBySelf || thread->completed < 4)
        return;

    // Track how long we've kept the same best move between iterations
    const uint16_t this_move = thread->pvs[thread->completed-0].line[0];
    const uint16_t last_move = thread->pvs[thread->completed-1].line[0];
    info->pv_stability = (this_move == last_move) ? MIN(10, info->pv_stability + 1) : 0;
}

bool terminateTimeManagment(const Thread *thread, const SearchInfo *info) {

    // Don't terminate early at very low depths
    if (thread->completed < 4) return FALSE;

    // Scale time between 80% and 120%, based on stable best moves
    const double pv_factor = 1.20 - 0.04 * info->pv_stability;

    // Scale time between 75% and 125%, based on score fluctuations
    const double score_change = thread->pvs[thread->completed-3].score
                              - thread->pvs[thread->completed-0].score;
    const double score_factor = MAX(0.75, MIN(1.25, 0.05 * score_change));

    //
    const uint64_t best_nodes = info->nodes[thread->pvs[thread->completed-0].line[0]];
    const double non_best_pct = 1.0 - ((double) best_nodes / thread->nodes);
    const double nodes_factor = MAX(0.50, 2 * non_best_pct + 0.4);

    return elapsedTime(info) > info->idealUsage * pv_factor * score_factor * nodes_factor;
}

bool terminateSearchEarly(const Thread *thread) {

    // Terminate the search early if the max usage time has passed.
    // Only check this once for every 1024 nodes examined, in case
    // the system calls are quite slow. Always be sure to avoid an
    // early exit during a depth 1 search, to ensure a best move

    const Limits *limits = thread->limits;

    if (limits->limitedByNodes)
        return thread->depth > 1
            && thread->nodes >= limits->nodeLimit / thread->nthreads;

    return  thread->depth > 1
        && (thread->nodes & 1023) == 1023
        && (limits->limitedBySelf || limits->limitedByTime)
        &&  elapsedTime(thread->info) >= thread->info->maxUsage;
}
