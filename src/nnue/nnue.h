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

#include <stdalign.h>
#include <stdint.h>

#include "misc.h"
#include "../bitboards.h"
#include "../types.h"

#define Position Board
#define Color int
#define Square int
#define Piece int
#define Value int
#define Bitboard uint64_t

#define pieces()         ((pos->colours[0] | pos->colours[1]))
#define pieces_p(x)      (pos->pieces[(x)])
#define piece_on(x)      (pos->squares[x])
#define type_of_p(x)     (pieceType((x)))
#define square_of(c, p)  (getlsb(pos->colours[c] & pos->pieces[p]))
#define make_piece(c, t) (makePiece((t), (c)))
#define stm()            (pos->turn)

#define pop_lsb(x)  (poplsb(x))

#define clamp(a, b, c) ((a) < (b) ? (b) : (a) > (c) ? (c) : (a))

void nnue_init(const char *fname);
Value nnue_evaluate(const Position *pos);
