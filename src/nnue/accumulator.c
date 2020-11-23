////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//    Ethereal is a UCI chess playing engine authored by Andrew Grant.        //
//    <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>        //
//                                                                            //
//    Ethereal is free software: you can redistribute it and/or modify        //
//    it under the terms of the GNU General Public License as published by    //
//    the Free Software Foundation, either version 3 of the License, or       //
//    (at your option) any later version.                                     //
//                                                                            //
//    Ethereal is distributed in the hope that it will be useful,             //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of          //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           //
//    GNU General Public License for more details.                            //
//                                                                            //
//    You should have received a copy of the GNU General Public License       //
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.   //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>

#include "types.h"
#include "nnue.h"

#include "../bitboards.h"
#include "../board.h"
#include "../thread.h"
#include "../types.h"

#include <immintrin.h>

extern NNUENetwork nnue;

static int nnue_index(Board *board, int relksq, int colour, int sq) {

    const int relsq   = relativeSquare(colour, sq);
    const int ptype   = pieceType(board->squares[sq]);
    const int pcolour = pieceColour(board->squares[sq]);

    return (640 * relksq) + (64 * (5 * (colour == pcolour) + ptype)) + (relsq);
}

static int nnue_index_delta(int piece, int relksq, int colour, int sq) {

    const int relsq   = relativeSquare(colour, sq);
    const int ptype   = pieceType(piece);
    const int pcolour = pieceColour(piece);

    return (640 * relksq) + (64 * (5 * (colour == pcolour) + ptype)) + (relsq);
}


void nnue_refresh_accumulator(NNUEAccumulator *accum, Board *board, int colour) {

    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    uint64_t pieces = (white | black) & ~kings;
    int relksq = relativeSquare(colour, getlsb(kings & board->colours[colour]));

    {
        const int index = nnue_index(board, relksq, colour, poplsb(&pieces));

        __m256* inputs  = (__m256*) &nnue.layers[0].weights[index * KPSIZE];
        __m256* outputs = (__m256*) &accum->values[colour][0];
        __m256* biases  = (__m256*) &nnue.layers[0].biases[0];

        for (int i = 0; i < KPSIZE / 8; i += 4) {
            outputs[i+0] = _mm256_add_ps(biases[i+0], inputs[i+0]);
            outputs[i+1] = _mm256_add_ps(biases[i+1], inputs[i+1]);
            outputs[i+2] = _mm256_add_ps(biases[i+2], inputs[i+2]);
            outputs[i+3] = _mm256_add_ps(biases[i+3], inputs[i+3]);
        }
    }

    while (pieces) {

        const int index = nnue_index(board, relksq, colour, poplsb(&pieces));

        __m256* inputs  = (__m256*) &nnue.layers[0].weights[index * KPSIZE];
        __m256* outputs = (__m256*) &accum->values[colour][0];

        for (int i = 0; i < KPSIZE / 8; i += 4) {
            outputs[i+0] = _mm256_add_ps(outputs[i+0], inputs[i+0]);
            outputs[i+1] = _mm256_add_ps(outputs[i+1], inputs[i+1]);
            outputs[i+2] = _mm256_add_ps(outputs[i+2], inputs[i+2]);
            outputs[i+3] = _mm256_add_ps(outputs[i+3], inputs[i+3]);
        }
    }
}

void nnue_update_accumulator(NNUEAccumulator *accum, Board *board) {

    const int height = board->thread->height;

    // At Root, or previous Node is inaccurate also
    if (height <= 0 || !(accum-1)->accurate) {
        nnue_refresh_accumulator(accum, board, WHITE);
        nnue_refresh_accumulator(accum, board, BLACK);
        return;
    }

    // Previous move was NULL from an accurate Node
    if ((accum-1)->accurate && !accum->changes) {
        memcpy(accum->values[WHITE], (accum-1)->values[WHITE], sizeof(float) * KPSIZE);
        memcpy(accum->values[BLACK], (accum-1)->values[BLACK], sizeof(float) * KPSIZE);
        return;
    }

    // ------------------------------------------------------------------------------------------

    NNUEDelta *deltas = accum->deltas;
    int add_list[2][32], remove_list[2][32];
    int add = 0, remove = 0, refreshed[2] = { 0, 0 };

    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    int wkrelsq = relativeSquare(WHITE, getlsb(white & kings));
    int bkrelsq = relativeSquare(BLACK, getlsb(black & kings));

    for (int i = 0; i < accum->changes; i++) {

        // Hard recompute a colour if their King has moved
        if (pieceType(deltas[i].piece) == KING) {
            nnue_refresh_accumulator(accum, board, pieceColour(deltas[i].piece));
            refreshed[pieceColour(deltas[i].piece)] = 1;
        }

        // A piece is being removed from the board outright
        else if (deltas[i].to == SQUARE_NB) {
            remove_list[WHITE][remove  ] = nnue_index_delta(deltas[i].piece, wkrelsq, WHITE, deltas[i].from);
            remove_list[BLACK][remove++] = nnue_index_delta(deltas[i].piece, bkrelsq, BLACK, deltas[i].from);
        }

        // A piece is being added to board (promotion)
        else if (deltas[i].from == SQUARE_NB) {
            add_list[WHITE][add  ] = nnue_index_delta(deltas[i].piece, wkrelsq, WHITE, deltas[i].to);
            add_list[BLACK][add++] = nnue_index_delta(deltas[i].piece, bkrelsq, BLACK, deltas[i].to);
        }

        // Typical piece movement results in an add and a remove for both sides
        else {

            add_list[WHITE][add  ] = nnue_index_delta(deltas[i].piece, wkrelsq, WHITE, deltas[i].to);
            add_list[BLACK][add++] = nnue_index_delta(deltas[i].piece, bkrelsq, BLACK, deltas[i].to);

            remove_list[WHITE][remove  ] = nnue_index_delta(deltas[i].piece, wkrelsq, WHITE, deltas[i].from);
            remove_list[BLACK][remove++] = nnue_index_delta(deltas[i].piece, bkrelsq, BLACK, deltas[i].from);
        }
    }

    // ------------------------------------------------------------------------------------------

    for (int colour = WHITE; colour <= BLACK; colour++) {

        if (refreshed[colour])
            continue;

        memcpy(accum->values[colour], (accum-1)->values[colour], sizeof(float) * KPSIZE);

        for (int i = 0; i < add; i++) {

            const int index = add_list[colour][i];
            __m256* inputs  = (__m256*) &nnue.layers[0].weights[index * KPSIZE];
            __m256* outputs = (__m256*) &accum->values[colour][0];

            for (int j = 0; j < KPSIZE / 8; j += 4) {
                outputs[j+0] = _mm256_add_ps(outputs[j+0], inputs[j+0]);
                outputs[j+1] = _mm256_add_ps(outputs[j+1], inputs[j+1]);
                outputs[j+2] = _mm256_add_ps(outputs[j+2], inputs[j+2]);
                outputs[j+3] = _mm256_add_ps(outputs[j+3], inputs[j+3]);
            }
        }

        for (int i = 0; i < remove; i++) {

            const int index = remove_list[colour][i];
            __m256* inputs  = (__m256*) &nnue.layers[0].weights[index * KPSIZE];
            __m256* outputs = (__m256*) &accum->values[colour][0];

            for (int j = 0; j < KPSIZE / 8; j += 4) {
                outputs[j+0] = _mm256_sub_ps(outputs[j+0], inputs[j+0]);
                outputs[j+1] = _mm256_sub_ps(outputs[j+1], inputs[j+1]);
                outputs[j+2] = _mm256_sub_ps(outputs[j+2], inputs[j+2]);
                outputs[j+3] = _mm256_sub_ps(outputs[j+3], inputs[j+3]);
            }
        }
    }

    // for (int i = 0; i < 16; i++)
    //     printf("%6.2f ", accum->values[WHITE][i]);
    // printf("\n");
    //
    // float A = accum->values[WHITE][0];
    //
    // nnue_refresh_accumulator(accum, board, WHITE);
    // nnue_refresh_accumulator(accum, board, BLACK);
    //
    // for (int i = 0; i < 16; i++)
    //     printf("%6.2f ", accum->values[WHITE][i]);
    // printf("\n");
    //
    // float B = accum->values[WHITE][0];
    //
    // if (A - B > 0.5)
    //     exit(EXIT_FAILURE);
}

