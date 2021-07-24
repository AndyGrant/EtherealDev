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

int MoveOverhead = 300; // Set by UCI options

extern volatile int ABORT_SIGNAL; // Global ABORT flag for threads
extern volatile int IS_PONDERING; // Global PONDER flag for threads

double get_real_time() {
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

double elapsed_time(Clock *clock) {
    return get_real_time() - clock->start_time;
}

void init_clock(Clock *clock, Limits *limits) {

    clock->start_time = limits->start; // Save off the start time of the search
    clock->pv_factor  = 0; // Clear our stability time usage heuristic

    // Allocate time if Ethereal is handling the clock
    if (limits->limitedBySelf) {

        // Playing using X / Y + Z time control
        if (limits->mtg >= 0) {
            clock->ideal_time     =  0.67 * (limits->time - MoveOverhead) / (limits->mtg +  5) + limits->inc;
            clock->max_alloc_time =  4.00 * (limits->time - MoveOverhead) / (limits->mtg +  7) + limits->inc;
            clock->max_usage_time = 10.00 * (limits->time - MoveOverhead) / (limits->mtg + 10) + limits->inc;
        }

        // Playing using X + Y time controls
        else {
            clock->ideal_time     =  0.90 * ((limits->time - MoveOverhead) + 25 * limits->inc) / 50;
            clock->max_alloc_time =  5.00 * ((limits->time - MoveOverhead) + 25 * limits->inc) / 50;
            clock->max_usage_time = 10.00 * ((limits->time - MoveOverhead) + 25 * limits->inc) / 50;
        }

        // Cap time allocations using the move overhead
        clock->ideal_time     = MIN(clock->ideal_time,     limits->time - MoveOverhead);
        clock->max_alloc_time = MIN(clock->max_alloc_time, limits->time - MoveOverhead);
        clock->max_usage_time = MIN(clock->max_usage_time, limits->time - MoveOverhead);
    }

    // Interface told us to search for a predefined duration
    else if (limits->limitedByTime) {
        clock->ideal_time     = limits->timeLimit;
        clock->max_alloc_time = limits->timeLimit;
        clock->max_usage_time = limits->timeLimit;
    }
}

void update_thread_clock(Thread *thread) {

    const int this_depth = thread->completed;
    const int this_value = thread->pvs[this_depth].score;
    const int last_value = thread->pvs[this_depth-1].score;

    // Don't update the self-clock at low depths
    if (!thread->limits->limitedBySelf || this_depth < 4) return;

    // Increase our time if the score suddenly dropped
    if (last_value > this_value + 10) thread->clock.ideal_time *= 1.050;
    if (last_value > this_value + 20) thread->clock.ideal_time *= 1.050;
    if (last_value > this_value + 40) thread->clock.ideal_time *= 1.050;

    // Increase our time if the score suddenly jumped
    if (last_value + 15 < this_value) thread->clock.ideal_time *= 1.025;
    if (last_value + 30 < this_value) thread->clock.ideal_time *= 1.050;

    // Scale back the PV time factor, but reset if the move changed
    thread->clock.pv_factor = MAX(0, thread->clock.pv_factor - 1);
    if (thread->pvs[this_depth].line[0] != thread->pvs[this_depth-1].line[0])
        thread->clock.pv_factor = PVFactorCount;
}

int terminate_thread_via_clock(Thread *thread) {

    // Adjust our ideal usage based on variance in the best move
    // between iterations of the search. We won't allow the new
    // usage value to exceed our maximum allocation. The cutoff
    // is reached if the elapsed time exceeds the ideal usage

    double cutoff = thread->clock.ideal_time;
    cutoff *= 1.00 + thread->clock.pv_factor * PVFactorWeight;
    return elapsed_time(&thread->clock) > MIN(cutoff, thread->clock.max_alloc_time);
}

int terminate_search_early(Thread *thread) {

    // Terminate the search early if the max usage time has passed.
    // Only check this once for every 1024 nodes examined, in case
    // the system calls are quite slow. Always be sure to avoid an
    // early exit during a depth 1 search, to ensure a best move

    const Limits *limits = thread->limits;

    if (ABORT_SIGNAL && thread->depth > 1)
        return 1;

    if (limits->limitedByNodes)
        return !IS_PONDERING
            &&  thread->depth > 1
            &&  thread->nodes >= limits->nodeLimit / thread->nthreads;

    return !IS_PONDERING
        &&  thread->depth > 1
        && (thread->nodes & 1023) == 1023
        && (thread->limits->limitedBySelf || thread->limits->limitedByTime)
        &&  elapsed_time(&thread->clock) >= thread->clock.max_usage_time;

}