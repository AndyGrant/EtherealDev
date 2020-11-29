/******************************************************************************/
/*                                                                            */
/*    Ethereal is a UCI chess playing engine authored by Andrew Grant.        */
/*    <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>        */
/*                                                                            */
/*    Ethereal is free software: you can redistribute it and/or modify        */
/*    it under the terms of the GNU General Public License as published by    */
/*    the Free Software Foundation, either version 3 of the License, or       */
/*    (at your option) any later version.                                     */
/*                                                                            */
/*    Ethereal is distributed in the hope that it will be useful,             */
/*    but WITHOUT ANY WARRANTY; without even the implied warranty of          */
/*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           */
/*    GNU General Public License for more details.                            */
/*                                                                            */
/*    You should have received a copy of the GNU General Public License       */
/*    along with this program.  If not, see <http://www.gnu.org/licenses/>.   */
/*                                                                            */
/******************************************************************************/

#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "accumulator.h"
#include "nnue.h"
#include "types.h"
#include "utils.h"

#include "../bitboards.h"
#include "../board.h"
#include "../thread.h"

ALIGN64 int16_t in_weights[INSIZE * KPSIZE ];
ALIGN64 int8_t  l1_weights[L1SIZE * L2SIZE ];
ALIGN64 int8_t  l2_weights[L2SIZE * L3SIZE ];
ALIGN64 int8_t  l3_weights[L3SIZE * OUTSIZE];

ALIGN64 int16_t in_biases[KPSIZE ];
ALIGN64 int32_t l1_biases[L2SIZE ];
ALIGN64 int32_t l2_biases[L3SIZE ];
ALIGN64 int32_t l3_biases[OUTSIZE];


INLINE void nnue_transpose(int8_t *matrix, int rows, int cols) {

    int8_t *cpy = malloc(sizeof(int8_t) * rows * cols);

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            cpy[j * rows + i] = matrix[i * cols + j];

    memcpy(matrix, cpy, sizeof(int8_t) * rows * cols);
    free(cpy);
}

INLINE void nnue_halfkp_relu(NNUEAccumulator *accum, int8_t *outputs, int length, int turn) {

    const __m256i zero  = _mm256_setzero_si256();
    const __m256i pmask = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);

    __m256i *in_white  = (__m256i *) &accum->values[WHITE];
    __m256i *in_black  = (__m256i *) &accum->values[BLACK];

    __m256i *out_white = (__m256i *) (turn == WHITE ? outputs : &outputs[KPSIZE]);
    __m256i *out_black = (__m256i *) (turn == BLACK ? outputs : &outputs[KPSIZE]);

    for (int i = 0; i < length / 32; i++) {
        __m256i pack1  = in_white[i*2+0], pack2 = in_white[i*2+1];
        __m256i packed = _mm256_packs_epi16(pack1, pack2);
        __m256i relued = _mm256_max_epi8(packed, zero);
        out_white[i]   = _mm256_permutevar8x32_epi32(relued, pmask);
    }

    for (int i = 0; i < length / 32; i++) {
        __m256i pack1  = in_black[i*2+0], pack2 = in_black[i*2+1];
        __m256i packed = _mm256_packs_epi16(pack1, pack2);
        __m256i relued = _mm256_max_epi8(packed, zero);
        out_black[i]   = _mm256_permutevar8x32_epi32(relued, pmask);
    }
}

INLINE void nnue_affine(int8_t *weights, int32_t *biases, int8_t *inputs, int32_t *outputs, int rows, int cols) {

    const int InChunks  = rows / 32;
    const int OutChunks = cols / 8;

    const __m256i ones  = _mm256_set1_epi16(1);

    const __m256i *inp = (__m256i *) inputs;
    const __m256i *bia = (__m256i *) biases;
    const __m256i *wgt = (__m256i *) weights;

    __m256i *out  = (__m256i*) outputs;

    for (int i = 0; i < OutChunks; i++) {

        __m256i acc0 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 0) + 0]);
        __m256i acc1 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 1) + 0]);
        __m256i acc2 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 2) + 0]);
        __m256i acc3 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 3) + 0]);

        for (int j = 1; j < InChunks; j++) {
            acc0 = _mm256_add_epi16(acc0, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 0) + j]));
            acc1 = _mm256_add_epi16(acc1, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 1) + j]));
            acc2 = _mm256_add_epi16(acc2, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 2) + j]));
            acc3 = _mm256_add_epi16(acc3, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 3) + j]));
        }

        acc0 = _mm256_madd_epi16(acc0, ones);
        acc1 = _mm256_madd_epi16(acc1, ones);
        acc2 = _mm256_madd_epi16(acc2, ones);
        acc3 = _mm256_madd_epi16(acc3, ones);


        __m256i acc4 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 4) + 0]);
        __m256i acc5 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 5) + 0]);
        __m256i acc6 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 6) + 0]);
        __m256i acc7 = _mm256_maddubs_epi16(inp[0], wgt[InChunks * (i * 8 + 7) + 0]);

        for (int j = 1; j < InChunks; j++) {
            acc4 = _mm256_add_epi16(acc4, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 4) + j]));
            acc5 = _mm256_add_epi16(acc5, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 5) + j]));
            acc6 = _mm256_add_epi16(acc6, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 6) + j]));
            acc7 = _mm256_add_epi16(acc7, _mm256_maddubs_epi16(inp[j], wgt[InChunks * (i * 8 + 7) + j]));
        }

        acc4 = _mm256_madd_epi16(acc4, ones);
        acc5 = _mm256_madd_epi16(acc5, ones);
        acc6 = _mm256_madd_epi16(acc6, ones);
        acc7 = _mm256_madd_epi16(acc7, ones);

        ///

        acc0 = _mm256_hadd_epi32(acc0, acc1);
        acc2 = _mm256_hadd_epi32(acc2, acc3);
        acc4 = _mm256_hadd_epi32(acc4, acc5);
        acc6 = _mm256_hadd_epi32(acc6, acc7);

        acc0 = _mm256_hadd_epi32(acc0, acc2);
        acc4 = _mm256_hadd_epi32(acc4, acc6);

        __m128i sumabcd1 = _mm256_extracti128_si256(acc0, 0);
        __m128i sumabcd2 = _mm256_extracti128_si256(acc0, 1);
        __m128i sumefgh1 = _mm256_extracti128_si256(acc4, 0);
        __m128i sumefgh2 = _mm256_extracti128_si256(acc4, 1);

        sumabcd1 = _mm_add_epi32(sumabcd1, sumabcd2);
        sumefgh1 = _mm_add_epi32(sumefgh1, sumefgh2);

        acc0 = _mm256_inserti128_si256(_mm256_castsi128_si256(sumabcd1), sumefgh1, 1);
        out[i] = _mm256_add_epi32(acc0, bia[i]);
    }
}

INLINE void nnue_relu(int32_t *inputs, int8_t *outputs, int length) {

    assert(length == 32); (void) length;

    const __m256i zero  = _mm256_setzero_si256();
    const __m256i pmask = _mm256_set_epi32(7, 3, 6, 2, 5, 1, 4, 0);

    __m256i *inp = (__m256i *) inputs;
    __m256i *out = (__m256i *) outputs;

    __m256i pack12 = _mm256_srai_epi16(_mm256_packs_epi32(inp[0], inp[1]), SHIFT);
    __m256i pack34 = _mm256_srai_epi16(_mm256_packs_epi32(inp[2], inp[3]), SHIFT);

    *out = _mm256_max_epi8(_mm256_packs_epi16(pack12, pack34), zero);
    *out = _mm256_permutevar8x32_epi32(*out, pmask);
}

INLINE int nnue_output(int8_t *weights, int32_t *biases, int8_t *inputs) {

    assert(L3SIZE == 32);

    const __m256i *inp  = (__m256i *) inputs;
    const __m256i *wgt  = (__m256i *) weights;

    const __m256i ones  = _mm256_set1_epi16(1);
    const __m256i sum16 = _mm256_maddubs_epi16(*inp, *wgt);
    const __m256i sum32 = _mm256_madd_epi16(sum16, ones);
    const __m256i upper = _mm256_permute2x128_si256(sum32, sum32, 1);

    const __m256i step1 = _mm256_hadd_epi32(sum32, upper);
    const __m256i step2 = _mm256_hadd_epi32(step1, step1);
    const __m256i step3 = _mm256_hadd_epi32(step2, step2);

    return (_mm256_extract_epi32(step3, 0) + *biases) / 16;
}


void load_nnue(const char* fname) {

    FILE *fin = fopen(fname, "rb");

    if (   fread(in_biases, sizeof(int16_t), KPSIZE, fin) != (size_t) KPSIZE
        || fread(in_weights, sizeof(int16_t), INSIZE * KPSIZE, fin) != (size_t) INSIZE * KPSIZE)
        printf("info string Unable to read NNUE\n"), fflush(stdout);

    if (   fread(l1_biases, sizeof(int32_t), L2SIZE, fin) != (size_t) L2SIZE
        || fread(l1_weights, sizeof(int8_t), L1SIZE * L2SIZE, fin) != (size_t) L1SIZE * L2SIZE)
        printf("info string Unable to read NNUE\n"), fflush(stdout);

    if (   fread(l2_biases, sizeof(int32_t), L3SIZE, fin) != (size_t) L3SIZE
        || fread(l2_weights, sizeof(int8_t), L2SIZE * L3SIZE, fin) != (size_t) L2SIZE * L3SIZE)
        printf("info string Unable to read NNUE\n"), fflush(stdout);

    if (   fread(l3_biases, sizeof(int32_t), OUTSIZE, fin) != (size_t) OUTSIZE
        || fread(l3_weights, sizeof(int8_t), L3SIZE * OUTSIZE, fin) != (size_t) L3SIZE * OUTSIZE)
        printf("info string Unable to read NNUE\n"), fflush(stdout);

    nnue_transpose(l1_weights, L1SIZE, L2SIZE);
    nnue_transpose(l2_weights, L2SIZE, L3SIZE);

    fclose(fin);
}

int nnue_evaluate(Thread *thread, Board *board) {

    // To perform some optimizations, auto-flag KvK as drawn to assume pieces >= 1
    if ((board->colours[WHITE] | board->colours[BLACK]) == board->pieces[KING])
        return 0;

    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    board->ksquares[WHITE] = getlsb(white & kings);
    board->ksquares[BLACK] = getlsb(black & kings);

    // Large enough to handle layer computations
    ALIGN64 int8_t  arr8[L1SIZE];
    ALIGN64 int32_t arr32[L1SIZE];

    NNUEAccumulator *accum = &thread->nnueStack[thread->height];

    if (!accum->accurate) {

        // Possible to recurse and incrementally update each
        if (nnue_can_update(accum, board))
            nnue_update_accumulator(accum, board);

        // History is missing, we must refresh completely
        else
            nnue_refresh_accumulators(accum, board);
    }

    nnue_halfkp_relu(accum, arr8, KPSIZE, board->turn);

    nnue_affine(l1_weights, l1_biases, arr8, arr32, L1SIZE, L2SIZE);
    nnue_relu(arr32, arr8, L2SIZE);

    nnue_affine(l2_weights, l2_biases, arr8, arr32, L2SIZE, L3SIZE);
    nnue_relu(arr32, arr8, L3SIZE);

    return nnue_output(l3_weights, l3_biases, arr8);
}
