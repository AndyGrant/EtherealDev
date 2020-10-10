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

#include "misc.h"

FD open_file(const char *fname) {
#ifndef _WIN32
    return open(fname, O_RDONLY);
#else
    return CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_RANDOM_ACCESS, NULL);
#endif
}

void close_file(FD fd) {
#ifndef _WIN32
    close(fd);
#else
    CloseHandle(fd);
#endif
}

const void *map_file(FD fd, map_t *map) {
#ifndef _WIN32
    *map = file_size(fd);
    void *data = mmap(NULL, *map, PROT_READ, MAP_SHARED, fd, 0);
    #ifdef MADV_RANDOM
    madvise(data, *map, MADV_RANDOM);
    #endif
    return data == MAP_FAILED ? NULL : data;
#else
    DWORD sizeLow, sizeHigh;
    sizeLow = GetFileSize(fd, &sizeHigh);
    *map = CreateFileMapping(fd, NULL, PAGE_READONLY, sizeHigh, sizeLow, NULL);
    if (*map == NULL)
        return NULL;
    return MapViewOfFile(*map, FILE_MAP_READ, 0, 0, 0);
#endif
}

void unmap_file(const void *data, map_t map) {
#ifndef _WIN32
    if (data)
        munmap((void *)data, map);
#else
    if (data) {
        UnmapViewOfFile(data);
        CloseHandle(map);
    }
#endif
}

size_t file_size(FD fd) {
#ifndef _WIN32
    struct stat statbuf;
    fstat(fd, &statbuf);
    return statbuf.st_size;
#else
    DWORD sizeLow, sizeHigh;
    sizeLow = GetFileSize(fd, &sizeHigh);
    return ((uint64_t)sizeHigh << 32) | sizeLow;
#endif
}

uint32_t readu_le_u32(const void *p) {
  const uint8_t *q = p;
  return q[0] | (q[1] << 8) | (q[2] << 16) | (q[3] << 24);
}

uint16_t readu_le_u16(const void *p) {
  const uint8_t *q = p;
  return q[0] | (q[1] << 8);
}
