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

#include <stdint.h>

#include "castle.h"
#include "types.h"

uint64_t ZobristTurn;
uint64_t ZobristCastle[0x10];
uint64_t ZobristEnpass[FILE_NB];
uint64_t Zobrist[32][SQUARE_NB];
uint64_t ZobristPawnKing[32][SQUARE_NB];

static uint64_t rand64(){

    static uint64_t seed = 1070372ull;

    seed ^= seed >> 12;
    seed ^= seed << 25;
    seed ^= seed >> 27;

    return seed * 2685821657736338717ull;
}

void initZobrist(){

    // Init Zobrist key for the side to move
    ZobristTurn = rand64();

    // Init Zobrist keys for individual castling rights
    ZobristCastle[WHITE_KING_RIGHTS]  = rand64();
    ZobristCastle[WHITE_QUEEN_RIGHTS] = rand64();
    ZobristCastle[BLACK_KING_RIGHTS]  = rand64();
    ZobristCastle[BLACK_QUEEN_RIGHTS] = rand64();

    // Init Zobrist keys for the combined castling rights
    for (int i = 0; i < 0x10; i++) {

        if (i & WHITE_KING_RIGHTS)
            ZobristCastle[i] ^= ZobristCastle[WHITE_KING_RIGHTS];

        if (i & WHITE_QUEEN_RIGHTS)
            ZobristCastle[i] ^= ZobristCastle[WHITE_QUEEN_RIGHTS];

        if (i & BLACK_KING_RIGHTS)
            ZobristCastle[i] ^= ZobristCastle[BLACK_KING_RIGHTS];

        if (i & BLACK_QUEEN_RIGHTS)
            ZobristCastle[i] ^= ZobristCastle[BLACK_QUEEN_RIGHTS];
    }

    // Init Zobrist keys for file containing enpass pawn
    for (int f = 0; f < RANK_NB; f++)
        ZobristEnpass[f] = rand64();

    // Init main general purpose Zobrist keys
    for (int c = WHITE; c <= BLACK; c++)
        for (int p = PAWN; p <= KING; p++)
            for (int s = 0; s < SQUARE_NB; s++)
                Zobrist[makePiece(p, c)][s] = rand64();

    // Init pawn-king hash Zobrist keys
    for (int c = WHITE; c <= BLACK; c++) {
        for (int s = 0; s < SQUARE_NB; s++) {
            ZobristPawnKing[makePiece(PAWN, c)][s] = rand64();
            ZobristPawnKing[makePiece(KING, c)][s] = rand64();
        }
    }
}