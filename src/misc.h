/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#ifdef _WIN32
    typedef HANDLE FD;
    #define FD_ERR INVALID_HANDLE_VALUE
    typedef HANDLE map_t;
#else
    typedef int FD;
    #define FD_ERR -1
    typedef size_t map_t;
#endif

FD open_file(const char *name);
void close_file(FD fd);
const void *map_file(FD fd, map_t *map);
void unmap_file(const void *data, map_t map);
size_t file_size(FD fd);

static inline bool is_little_endian(void) {
  int num = 1;
  return *(uint8_t *)&num == 1;
}

static inline uint32_t from_le_u32(uint32_t v) {
  return is_little_endian() ? v : __builtin_bswap32(v);
}

static inline uint16_t from_le_u16(uint16_t v) {
  return is_little_endian() ? v : __builtin_bswap16(v);
}

static inline uint64_t from_be_u64(uint64_t v) {
  return is_little_endian() ? __builtin_bswap64(v) : v;
}

static inline uint32_t from_be_u32(uint32_t v) {
  return is_little_endian() ? __builtin_bswap32(v) : v;
}

static inline uint16_t from_be_u16(uint16_t v) {
  return is_little_endian() ? __builtin_bswap16(v) : v;
}

static inline uint32_t read_le_u32(const void *p) {
  return from_le_u32(*(uint32_t *)p);
}

static inline uint16_t read_le_u16(const void *p) {
  return from_le_u16(*(uint16_t *)p);
}

static inline uint32_t readu_le_u32(const void *p) {
  const uint8_t *q = p;
  return q[0] | (q[1] << 8) | (q[2] << 16) | (q[3] << 24);
}

static inline uint16_t readu_le_u16(const void *p) {
  const uint8_t *q = p;
  return q[0] | (q[1] << 8);
}
