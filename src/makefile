# Ethereal is a UCI chess playing engine authored by Andrew Grant.
# <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>
#
# Ethereal is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Ethereal is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

########################################################################################
# 1. General Configuration
########################################################################################

CC      = gcc
SRC     = *.c pyrrhic/tbprobe.c
LIBS    = -lpthread -lm
EXE     = Ethereal

WFLAGS = -std=gnu11 -s -Wall -Wextra -Wshadow -DNNUE_EMBEDDED
CFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -march=native
RFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -static
TFLAGS = -O3 $(WFLAGS) -DNDEBUG -flto -march=native -fopenmp -DTUNE
PFLAGS = -O0 $(WFLAGS) -DNDEBUG -p -pg
DFLAGS = -O0 $(WFLAGS)

POPCNTFLAGS = -DUSE_POPCNT -msse3 -mpopcnt
PEXTFLAGS   = $(POPCNTFLAGS) -DUSE_PEXT -mbmi2

ARMV8FLAGS  = -O3 $(WFLAGS) -DNDEBUG -flto -march=armv8-a -m64
ARMV7FLAGS  = -O3 $(WFLAGS) -DNDEBUG -flto -march=armv7-a -m32
ARMV7FLAGS += -mfloat-abi=softfp -mfpu=vfpv3-d16 -mthumb -Wl,--fix-cortex-a8

########################################################################################
# 2. Special Purpose Instruction Sets for NNUE
########################################################################################

PROPERTIES = $(shell echo | $(CC) -m64 -march=native -E -dM -)

ifneq ($(findstring __AVX2__, $(PROPERTIES)),)
	CFLAGS += -DUSE_AVX2
endif

ifneq ($(findstring __SSE41__, $(PROPERTIES)),)
	CFLAGS += -DUSE_SSE41
endif

ifneq ($(findstring __SSSE3__, $(PROPERTIES)),)
	CFLAGS += -DUSE_SSSE3
endif

ifneq ($(findstring __SSE2__, $(PROPERTIES)),)
	CFLAGS += -DUSE_SSE2
endif

ifneq ($(findstring __SSE__, $(PROPERTIES)),)
	CFLAGS += -DUSE_SSE
endif

ifneq ($(findstring __MMX__, $(PROPERTIES)),)
	CFLAGS += -DUSE_MMX
endif

########################################################################################
# 3. Targets
########################################################################################

popcnt:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) -o $(EXE)

nopopcnt:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) -o $(EXE)

pext:
	$(CC) $(CFLAGS) $(SRC) $(LIBS) $(PEXTFLAGS) -o $(EXE)

release:
	mkdir ../dist
	$(CC) $(RFLAGS) $(SRC) $(LIBS) -o ../dist/$(EXE)$(VER)-x64-nopopcnt.exe
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(POPCNTFLAGS) -o ../dist/$(EXE)$(VER)-x64-popcnt.exe
	$(CC) $(RFLAGS) $(SRC) $(LIBS) $(PEXTFLAGS) -o ../dist/$(EXE)$(VER)-x64-pext.exe

tune:
	$(CC) $(TFLAGS) $(SRC) $(LIBS) $(POPCNT) -o $(EXE)

profile:
	$(CC) $(PFLAGS) $(SRC) $(LIBS) $(POPCNT) -o $(EXE)

debug:
	$(CC) $(DFLAGS) $(SRC) $(LIBS) $(POPCNT) -o $(EXE)

armv8:
	$(CC) $(ARMV8FLAGS) $(SRC) -lm -o $(EXE)

armv7:
	$(CC) $(ARMV7FLAGS) $(SRC) -lm -o $(EXE)
