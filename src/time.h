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

#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#else
    #include <sys/time.h>
#endif

#include "types.h"

struct Clock {
    double start_time;
    double ideal_time;
    double max_alloc_time;
    double max_usage_time;
    int pv_factor;
};

double get_real_time();
double elapsed_time(Clock *clock);
void init_clock(Clock *clock, Limits *limits);
void update_thread_clock(Thread *thread);
int terminate_thread_via_clock(Thread *thread);
int terminate_search_early(Thread *thread);

static const double PVFactorCount  = 9;
static const double PVFactorWeight = 0.105;
