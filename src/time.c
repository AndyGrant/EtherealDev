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

int expectedToExceedTime(SearchInfo* info, int depth){
    
    if (depth < 8) return 0; // Time usage heuristics are rather poor at low depths
        
    // Take the highest time factor over the last 4 search iterations
    double tf1 = info->timeUsage[depth-0] / MAX(50, info->timeUsage[depth-1]);
    double tf2 = info->timeUsage[depth-1] / MAX(50, info->timeUsage[depth-2]);
    double tf3 = info->timeUsage[depth-2] / MAX(50, info->timeUsage[depth-3]);
    double tfN = MAX(tf1, MAX(tf2, tf3));
    
    // Estimate time usage with the greatest factor plus a buffer
    double estimatedUsage = info->timeUsage[depth] * (tfN + 0.50);
    
    // Adjust usage to match the start time of the search
    double estiamtedEndtime = getRealTime() + estimatedUsage - info->starttime;
    
    // Return whether or not we expect to run out of time this search
    return estiamtedEndtime > info->maxusage;
}