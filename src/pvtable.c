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

#include "thread.h"
#include "pvtable.h"
#include "move.h"

uint16_t probe_pv_table(Thread *thread, uint64_t hash64) {

    const uint16_t lookup16 = hash64 & 0xFFFF;
    const uint16_t check16  = hash64 >> 48;

    return thread->pvtable.table[lookup16].hash16 == check16
         ? thread->pvtable.table[lookup16].move : NONE_MOVE;
}

void store_pv_table(Thread * thread, uint64_t hash64, uint16_t move) {

    const uint16_t lookup16 = hash64 & 0xFFFF;
    const uint16_t check16  = hash64 >> 48;

    thread->pvtable.table[lookup16].hash16 = check16;
    thread->pvtable.table[lookup16].move   = move;
}
