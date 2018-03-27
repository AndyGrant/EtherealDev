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
#include "uci.h"

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

double terminateSearchHere(SearchInfo* info, Limits* limits, int depth){
    
    double timeFactor, estimatedUsage, estiamtedEndtime;

    // Check for termination by the defined UCI, or self defined time managment conditions
    if (   (limits->limitedByDepth && depth >= limits->depthLimit)
        || (limits->limitedByTime  && getRealTime() - info->starttime > limits->timeLimit)
        || (limits->limitedBySelf  && getRealTime() - info->starttime > info->maxusage)
        || (limits->limitedBySelf  && getRealTime() - info->starttime > info->idealusage))
        return 1;
    
    // Check to see if we expect to be able to complete the next depth
    if (limits->limitedBySelf && depth >= 8){
        
        timeFactor       = info->timeUsage[depth] / MAX(1, info->timeUsage[depth-1]);
        estimatedUsage   = info->timeUsage[depth] * (timeFactor + .40);
        estiamtedEndtime = getRealTime() + estimatedUsage - info->starttime;
        
        if (estiamtedEndtime > info->maxusage)
            return 1;
    }
    
    return 0;
}
