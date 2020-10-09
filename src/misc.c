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

#include <stdlib.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include "misc.h"
#include "thread.h"

#ifndef _WIN32
pthread_mutex_t ioMutex = PTHREAD_MUTEX_INITIALIZER;
#else
HANDLE ioMutex;
#endif

ssize_t getline(char **lineptr, size_t *n, FILE *stream)
{
  if (*n == 0)
    *lineptr = malloc(*n = 100);

  int c = 0;
  size_t i = 0;
  while ((c = getc(stream)) != EOF) {
    (*lineptr)[i++] = c;
    if (i == *n)
      *lineptr = realloc(*lineptr, *n += 100);
    if (c == '\n') break;
  }
  (*lineptr)[i] = 0;
  return i;
}

#ifdef _WIN32
// The following two functions were taken from mingw_lock.c
void __cdecl _lock(int locknum);
void __cdecl _unlock(int locknum);
#define _STREAM_LOCKS 16
#define _IOLOCKED 0x8000
typedef struct {
  FILE f;
  CRITICAL_SECTION lock;
} _FILEX;

void flockfile(FILE *F)
{
  if ((F >= (&__iob_func()[0])) && (F <= (&__iob_func()[_IOB_ENTRIES-1]))) {
    _lock(_STREAM_LOCKS + (int)(F - (&__iob_func()[0])));
    F->_flag |= _IOLOCKED;
  } else
    EnterCriticalSection(&(((_FILEX *)F)->lock));
}

void funlockfile(FILE *F)
{
  if ((F >= (&__iob_func()[0])) && (F <= (&__iob_func()[_IOB_ENTRIES-1]))) {
    F->_flag &= ~_IOLOCKED;
    _unlock(_STREAM_LOCKS + (int)(F - (&__iob_func()[0])));
  } else
    LeaveCriticalSection(&(((_FILEX *)F)->lock));
}
#endif

FD open_file(const char *name)
{
#ifndef _WIN32
  return open(name, O_RDONLY);
#else
  return CreateFile(name, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
      FILE_FLAG_RANDOM_ACCESS, NULL);
#endif
}

void close_file(FD fd)
{
#ifndef _WIN32
  close(fd);
#else
  CloseHandle(fd);
#endif
}

size_t file_size(FD fd)
{
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

const void *map_file(FD fd, map_t *map)
{
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

void unmap_file(const void *data, map_t map)
{
  if (!data) return;

#ifndef _WIN32

  munmap((void *)data, map);

#else

  UnmapViewOfFile(data);
  CloseHandle(map);

#endif
}
