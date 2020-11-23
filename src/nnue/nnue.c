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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../bitboards.h"
#include "../thread.h"

#include "accumulator.h"
#include "nnue.h"
#include "utils.h"
#include "types.h"

#include <immintrin.h>

NNUENetwork nnue = (NNUENetwork) {
    (NNUELayer[]) {
        {   40960,  KPSIZE, NULL, NULL},
        {  L1SIZE,  L2SIZE, NULL, NULL},
        {  L2SIZE,  L3SIZE, NULL, NULL},
        {  L3SIZE,       1, NULL, NULL},
    }
};


INLINE void nnue_halfkp_relu(NNUEAccumulator *accum, float *outputs, int length, int turn) {

    const __m256 zero = _mm256_setzero_ps();

    __m256 *in_white  = (__m256 *) &accum->values[WHITE];
    __m256 *in_black  = (__m256 *) &accum->values[BLACK];

    __m256 *out_white = (__m256 *) (turn == WHITE ? outputs : &outputs[KPSIZE]);
    __m256 *out_black = (__m256 *) (turn == BLACK ? outputs : &outputs[KPSIZE]);

    for (int i = 0; i < length / 8; i += 8) {
        out_white[i+0] = _mm256_max_ps(zero, in_white[i+0]);
        out_white[i+1] = _mm256_max_ps(zero, in_white[i+1]);
        out_white[i+2] = _mm256_max_ps(zero, in_white[i+2]);
        out_white[i+3] = _mm256_max_ps(zero, in_white[i+3]);
        out_white[i+4] = _mm256_max_ps(zero, in_white[i+4]);
        out_white[i+5] = _mm256_max_ps(zero, in_white[i+5]);
        out_white[i+6] = _mm256_max_ps(zero, in_white[i+6]);
        out_white[i+7] = _mm256_max_ps(zero, in_white[i+7]);
    }

    for (int i = 0; i < length / 8; i += 8) {
        out_black[i+0] = _mm256_max_ps(zero, in_black[i+0]);
        out_black[i+1] = _mm256_max_ps(zero, in_black[i+1]);
        out_black[i+2] = _mm256_max_ps(zero, in_black[i+2]);
        out_black[i+3] = _mm256_max_ps(zero, in_black[i+3]);
        out_black[i+4] = _mm256_max_ps(zero, in_black[i+4]);
        out_black[i+5] = _mm256_max_ps(zero, in_black[i+5]);
        out_black[i+6] = _mm256_max_ps(zero, in_black[i+6]);
        out_black[i+7] = _mm256_max_ps(zero, in_black[i+7]);
    }
}

INLINE void nnue_affine_relu(float *weights, float *biases, float *inputs, float *outputs, int rows, int cols) {

    /// Solution Derived from https://github.com/Luecx/Koivisto

    const int InChunks  = rows / 8;
    const int OutChunks = cols / 8;

    const __m256 zero = _mm256_setzero_ps();

    const __m256 *inp = (__m256 *) inputs;
    const __m256 *bia = (__m256 *) biases;
    const __m256 *wgt = (__m256 *) weights;
    __m256 *const out = (__m256 *) outputs;

    for (int i = 0; i < OutChunks; i++) {

        __m256 acc0 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 0) + 0], inp[0]);
        __m256 acc1 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 1) + 0], inp[0]);
        __m256 acc2 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 2) + 0], inp[0]);
        __m256 acc3 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 3) + 0], inp[0]);
        __m256 acc4 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 4) + 0], inp[0]);
        __m256 acc5 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 5) + 0], inp[0]);
        __m256 acc6 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 6) + 0], inp[0]);
        __m256 acc7 = _mm256_mul_ps(wgt[InChunks * (i * 8 + 7) + 0], inp[0]);

        for (int j = 1; j < InChunks; j++) {
            acc0 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 0) + j], inp[j], acc0);
            acc1 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 1) + j], inp[j], acc1);
            acc2 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 2) + j], inp[j], acc2);
            acc3 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 3) + j], inp[j], acc3);
            acc4 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 4) + j], inp[j], acc4);
            acc5 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 5) + j], inp[j], acc5);
            acc6 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 6) + j], inp[j], acc6);
            acc7 = _mm256_fmadd_ps(wgt[InChunks * (i * 8 + 7) + j], inp[j], acc7);
        }

        acc0 = _mm256_hadd_ps(acc0, acc1);
        acc2 = _mm256_hadd_ps(acc2, acc3);
        acc4 = _mm256_hadd_ps(acc4, acc5);
        acc6 = _mm256_hadd_ps(acc6, acc7);

        acc0 = _mm256_hadd_ps(acc0, acc2);
        acc4 = _mm256_hadd_ps(acc4, acc6);

        __m128 sumabcd1 = _mm256_extractf128_ps(acc0, 0);
        __m128 sumabcd2 = _mm256_extractf128_ps(acc0, 1);
        __m128 sumefgh1 = _mm256_extractf128_ps(acc4, 0);
        __m128 sumefgh2 = _mm256_extractf128_ps(acc4, 1);

        sumabcd1 = _mm_add_ps(sumabcd1, sumabcd2);
        sumefgh1 = _mm_add_ps(sumefgh1, sumefgh2);

        acc0 = _mm256_insertf128_ps(_mm256_castps128_ps256(sumabcd1), sumefgh1, 1);
        out[i] = _mm256_max_ps(zero, _mm256_add_ps(bia[i], acc0));
    }
}

INLINE void nnue_output_transform(float *weights, float *biases, float *inputs, float *outputs, int rows) {

    const int InChunks = rows / 8;
    const __m256 *inp  = (__m256 *) inputs;
    const __m256 *wgt  = (__m256 *) weights;

    __m256 acc = _mm256_mul_ps(wgt[0], inp[0]);
    for (int i = 1; i < InChunks; i++)
        acc = _mm256_fmadd_ps(wgt[i], inp[i], acc);

    const __m128 hiQuad  = _mm256_extractf128_ps(acc, 1);
    const __m128 loQuad  = _mm256_castps256_ps128(acc);
    const __m128 sumQuad = _mm_add_ps(loQuad, hiQuad);

    const __m128 hiDual  = _mm_movehl_ps(sumQuad, sumQuad);
    const __m128 sumDual = _mm_add_ps(sumQuad, hiDual);

    const __m128 hi      = _mm_shuffle_ps(sumDual, sumDual, 0x1);
    const __m128 sum     = _mm_add_ss(sumDual, hi);

    *outputs = _mm_cvtss_f32(sum) + *biases;
}


void load_nnue(const char* fname) {

    FILE *fin = fopen(fname, "rb");

    for (int i = 0; i < LAYERS; i++) {

        const int rows = nnue.layers[i].rows;
        const int cols = nnue.layers[i].cols;

        if (nnue.layers[i].weights)
            align_free(nnue.layers[i].weights);
        nnue.layers[i].weights = align_malloc(sizeof(float) * rows * cols);

        if (nnue.layers[i].biases)
            align_free(nnue.layers[i].biases);
        nnue.layers[i].biases = align_malloc(sizeof(float) * cols);

        if (   fread(nnue.layers[i].biases, sizeof(float), cols, fin) != (size_t) cols
            || fread(nnue.layers[i].weights, sizeof(float), rows * cols, fin) != (size_t) rows * cols)
            printf("info string Unable to read NNUE\n"), fflush(stdout);
    }

    nnue_transpose(nnue.layers[1].weights, nnue.layers[1].rows, nnue.layers[1].cols);
    nnue_transpose(nnue.layers[2].weights, nnue.layers[2].rows, nnue.layers[2].cols);

    fclose(fin);
}

int nnue_evaluate(Thread *thread, Board *board) {

    // To perform some optimizations, auto-flag KvK as drawn to assume pieces >= 1
    if ((board->colours[WHITE] | board->colours[BLACK]) == board->pieces[KING])
        return 0;

    // Large enough to handle layer computations
    ALIGN64 float outN1[L1SIZE], outN2[L1SIZE];
    NNUEAccumulator *accum = &thread->nnueStack[thread->height];

    // Update the accumulated L1 Neurons if they are expired
    if (!accum->accurate)
        nnue_update_accumulator(accum, board);

    nnue_halfkp_relu(accum, outN2, KPSIZE, board->turn);
    nnue_affine_relu(nnue.layers[1].weights, nnue.layers[1].biases, outN2, outN1, L1SIZE, L2SIZE);
    nnue_affine_relu(nnue.layers[2].weights, nnue.layers[2].biases, outN1, outN2, L2SIZE, L3SIZE);
    nnue_output_transform(nnue.layers[3].weights, nnue.layers[3].biases, outN2, outN1, L3SIZE);

    return outN1[0];
}

void nnue_transpose(float *matrix, int rows, int cols) {

    float *cpy = malloc(sizeof(float) * rows * cols);

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            cpy[j * rows + i] = matrix[i * cols + j];

    memcpy(matrix, cpy, sizeof(float) * rows * cols);
    free(cpy);
}
