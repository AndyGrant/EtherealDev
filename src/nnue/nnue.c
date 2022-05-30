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
/*    along with this program.  If not, see <http://www.gnu.org/licenses/>    */
/*                                                                            */
/******************************************************************************/

#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>

#include "accumulator.h"
#include "nnue.h"
#include "types.h"
#include "utils.h"

#include "../bitboards.h"
#include "../board.h"
#include "../evaluate.h"
#include "../thread.h"

#include "../incbin/incbin.h"

#define SHIFT_L0 6
#define SHIFT_L1 5

#ifdef EVALFILE
const char *NNUEDefault = EVALFILE;
INCBIN(IncWeights, EVALFILE);
#endif

ALIGN64 int16_t in_weights[INSIZE * KPSIZE ];
ALIGN64 int8_t  l1_weights[L1SIZE * OUTSIZE];
ALIGN64 int16_t in_biases[KPSIZE ];
ALIGN64 int32_t l1_biases[OUTSIZE];

static int NNUE_LOADED = 0;


static void shuffle_input_layer() {

    #if defined(USE_AVX2)

    __m256i *wgt = (__m256i *) in_weights;
    __m256i *bia = (__m256i *) in_biases;

    // Interleave adjacent 256-bit chunks of 2-byte values. During
    // halfkp_relu() adjacent chunks are split, with the A-half of
    // chunk 1 swapping with A-half of chunk 2. This is done to both
    // the weights and the biases, to avoid unshuffling them later.

    for (int i = 0; i < KPSIZE / vepi16_cnt; i += 2) {

        __m128i half1 = _mm256_extracti128_si256(bia[i+0], 1);
        __m128i half2 = _mm256_extracti128_si256(bia[i+1], 0);

        bia[i+0] = _mm256_inserti128_si256(bia[i+0], half2, 1);
        bia[i+1] = _mm256_inserti128_si256(bia[i+1], half1, 0);
    }

    for (int i = 0; i < INSIZE * KPSIZE / vepi16_cnt; i += 2) {

        __m128i half1 = _mm256_extracti128_si256(wgt[i+0], 1);
        __m128i half2 = _mm256_extracti128_si256(wgt[i+1], 0);

        wgt[i+0] = _mm256_inserti128_si256(wgt[i+0], half2, 1);
        wgt[i+1] = _mm256_inserti128_si256(wgt[i+1], half1, 0);
    }

    #endif
}

static void abort_nnue(const char *reason) {
    printf("info string %s\n", reason);
    fflush(stdout); exit(EXIT_FAILURE);
}


INLINE void halfkp_relu(NNUEAccumulator *accum, uint8_t *outputs, int turn) {

    // The accumulation of king-piece values has already been computed.
    // Perform the ReLU operation on each accumuatlor, and place them
    // such that the side-to-move is first, then the non-side-to-move

    assert(KPSIZE % 64 == 0);

    vepi16 *in_white = (vepi16 *) &accum->values[WHITE];
    vepi16 *in_black = (vepi16 *) &accum->values[BLACK];

    vepi8 *out_white = (vepi8 *) (turn == WHITE ? outputs : &outputs[KPSIZE]);
    vepi8 *out_black = (vepi8 *) (turn == BLACK ? outputs : &outputs[KPSIZE]);

    for (int i = 0; i < KPSIZE / vepi8_cnt; i += 2) {

        vepi16 shift0A = vepi16_srai(in_white[(i + 0) * 2 + 0], SHIFT_L0);
        vepi16 shift0B = vepi16_srai(in_white[(i + 0) * 2 + 1], SHIFT_L0);
        vepi16 shift1A = vepi16_srai(in_white[(i + 1) * 2 + 0], SHIFT_L0);
        vepi16 shift1B = vepi16_srai(in_white[(i + 1) * 2 + 1], SHIFT_L0);

        out_white[i+0] = vepi16_packu(shift0A, shift0B);
        out_white[i+1] = vepi16_packu(shift1A, shift1B);
    }

    for (int i = 0; i < KPSIZE / vepi8_cnt; i += 2) {

        vepi16 shift0A = vepi16_srai(in_black[(i + 0) * 2 + 0], SHIFT_L0);
        vepi16 shift0B = vepi16_srai(in_black[(i + 0) * 2 + 1], SHIFT_L0);
        vepi16 shift1A = vepi16_srai(in_black[(i + 1) * 2 + 0], SHIFT_L0);
        vepi16 shift1B = vepi16_srai(in_black[(i + 1) * 2 + 1], SHIFT_L0);

        out_black[i+0] = vepi16_packu(shift0A, shift0B);
        out_black[i+1] = vepi16_packu(shift1A, shift1B);
    }
}

INLINE int output_transform(int8_t *weights, int32_t *biases, uint8_t *inputs) {

    const vepi16 ones = vepi16_one;
    const vepi8  *inp = (vepi8 *) inputs;
    const vepi8  *wgt = (vepi8 *) weights;

    vepi32 acc = vepi32_zero();

    for (int i = 0; i < L1SIZE / vepi8_cnt; i += 2) {
        const vepi16 sumA = vepi16_maubs(inp[i+0], wgt[i + 0]);
        const vepi16 sumB = vepi16_maubs(inp[i+1], wgt[i + 1]);
        acc = vepi32_add(acc, vepi16_madd(ones, vepi16_add(sumA, sumB)));
    }

    const __m128i r4 = _mm_add_epi32(_mm256_castsi256_si128(acc), _mm256_extractf128_si256(acc, 1));
    const __m128i r2 = _mm_add_epi32(r4, _mm_srli_si128(r4, 8));
    const __m128i r1 = _mm_add_epi32(r2, _mm_srli_si128(r2, 4));

    return _mm_cvtsi128_si32(r1) + *biases;

    // int output = *biases;
    //
    // for (int i = 0; i < L1SIZE; i++)
    //     output += weights[i] * inputs[i];
    //
    // return output;

    /*assert(L1SIZE % 16 == 0 && L2SIZE % 8 == 0);

    const int InChunks  = L1SIZE / vepi8_cnt;
    const int OutChunks = L2SIZE / 8;

    #if defined(USE_AVX2) || defined(USE_AVX)
    const vepi32 zero = vepi32_zero();
    #elif defined(USE_SSSE3)
    const vps32  zero = vps32_zero();
    #endif

    const vepi16 ones = vepi16_one;

    const vepi8  *inp = (vepi8  *) inputs;
    const vepi8  *wgt = (vepi8  *) weights;
    const vepi32 *bia = (vepi32 *) biases;
    vps32 *const out  = (vps32  *) outputs;

    for (int i = 0; i < OutChunks; i++) {

        vepi32 acc0 = vepi32_zero();
        vepi32 acc1 = vepi32_zero();
        vepi32 acc2 = vepi32_zero();
        vepi32 acc3 = vepi32_zero();
        vepi32 acc4 = vepi32_zero();
        vepi32 acc5 = vepi32_zero();
        vepi32 acc6 = vepi32_zero();
        vepi32 acc7 = vepi32_zero();

        for (int j = 0; j < InChunks; j += 2) {

            vepi16 sum0A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 0) + j + 0]);
            vepi16 sum0B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 0) + j + 1]);
            acc0 = vepi32_add(acc0, vepi16_madd(ones, vepi16_add(sum0A, sum0B)));

            vepi16 sum1A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 1) + j + 0]);
            vepi16 sum1B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 1) + j + 1]);
            acc1 = vepi32_add(acc1, vepi16_madd(ones, vepi16_add(sum1A, sum1B)));

            vepi16 sum2A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 2) + j + 0]);
            vepi16 sum2B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 2) + j + 1]);
            acc2 = vepi32_add(acc2, vepi16_madd(ones, vepi16_add(sum2A, sum2B)));

            vepi16 sum3A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 3) + j + 0]);
            vepi16 sum3B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 3) + j + 1]);
            acc3 = vepi32_add(acc3, vepi16_madd(ones, vepi16_add(sum3A, sum3B)));

            vepi16 sum4A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 4) + j + 0]);
            vepi16 sum4B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 4) + j + 1]);
            acc4 = vepi32_add(acc4, vepi16_madd(ones, vepi16_add(sum4A, sum4B)));

            vepi16 sum5A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 5) + j + 0]);
            vepi16 sum5B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 5) + j + 1]);
            acc5 = vepi32_add(acc5, vepi16_madd(ones, vepi16_add(sum5A, sum5B)));

            vepi16 sum6A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 6) + j + 0]);
            vepi16 sum6B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 6) + j + 1]);
            acc6 = vepi32_add(acc6, vepi16_madd(ones, vepi16_add(sum6A, sum6B)));

            vepi16 sum7A = vepi16_maubs(inp[j+0], wgt[InChunks * (i * 8 + 7) + j + 0]);
            vepi16 sum7B = vepi16_maubs(inp[j+1], wgt[InChunks * (i * 8 + 7) + j + 1]);
            acc7 = vepi32_add(acc7, vepi16_madd(ones, vepi16_add(sum7A, sum7B)));
        }


        acc0 = vepi32_hadd(acc0, acc1);
        acc2 = vepi32_hadd(acc2, acc3);
        acc0 = vepi32_hadd(acc0, acc2);
        acc4 = vepi32_hadd(acc4, acc5);
        acc6 = vepi32_hadd(acc6, acc7);
        acc4 = vepi32_hadd(acc4, acc6);

        #if defined(USE_AVX2)

        __m128i sumabcd1 = _mm256_extracti128_si256(acc0, 0);
        __m128i sumabcd2 = _mm256_extracti128_si256(acc0, 1);
        __m128i sumefgh1 = _mm256_extracti128_si256(acc4, 0);
        __m128i sumefgh2 = _mm256_extracti128_si256(acc4, 1);

        sumabcd1 = _mm_add_epi32(sumabcd1, sumabcd2);
        sumefgh1 = _mm_add_epi32(sumefgh1, sumefgh2);

        acc0 = _mm256_inserti128_si256(_mm256_castsi128_si256(sumabcd1), sumefgh1, 1);
        acc0 = _mm256_add_epi32(acc0, bia[i]);
        acc0 = _mm256_max_epi32(acc0, zero);
        out[i] = _mm256_cvtepi32_ps(acc0);

        #elif defined (USE_AVX)

        __m128 ps0 = _mm_cvtepi32_ps(vepi32_max(zero, vepi32_add(bia[i * 2 + 0], acc0)));
        __m128 ps1 = _mm_cvtepi32_ps(vepi32_max(zero, vepi32_add(bia[i * 2 + 1], acc4)));

        out[i] = _mm256_insertf128_ps(out[i], ps0, 0);
        out[i] = _mm256_insertf128_ps(out[i], ps1, 1);

        #elif defined (USE_SSSE3)

        out[i * 2 + 0] = vps32_max(zero, _mm_cvtepi32_ps(vepi32_add(bia[i * 2 + 0], acc0)));
        out[i * 2 + 1] = vps32_max(zero, _mm_cvtepi32_ps(vepi32_add(bia[i * 2 + 1], acc4)));

        #endif
    }*/
}


void nnue_init(const char* fname) {

    FILE *fin = fopen(fname, "rb");

    if (   fread(in_biases, sizeof(int16_t), KPSIZE, fin) != (size_t) KPSIZE
        || fread(in_weights, sizeof(int16_t), INSIZE * KPSIZE, fin) != (size_t) INSIZE * KPSIZE)
        abort_nnue("Unable to read NNUE File");

    if (   fread(l1_biases, sizeof(int32_t), OUTSIZE, fin) != (size_t) OUTSIZE
        || fread(l1_weights, sizeof(int8_t), L1SIZE * OUTSIZE, fin) != (size_t) L1SIZE * OUTSIZE)
        abort_nnue("Unable to read NNUE File");

    shuffle_input_layer();
    fclose(fin);

    NNUE_LOADED = 1;
}

void nnue_incbin_init() {

    #ifdef EVALFILE

    int8_t *data8; int16_t *data16; int32_t *data32;

    // Input layer uses 16-bit Biases and Weights

    data16 = (int16_t*) gIncWeightsData;
    for (int i = 0; i < KPSIZE; i++)
        in_biases[i] = *(data16++);

    for (int i = 0; i < INSIZE * KPSIZE; i++)
        in_weights[i] = *(data16++);

    // Layer one uses 32-bit Biases and 8-bit Weights

    data32 = (int32_t*) data16;
    for (int i = 0; i < OUTSIZE; i++)
        l1_biases[i] = *(data32++);

    data8 = (int8_t*) data32;
    for (int i = 0; i < L1SIZE * OUTSIZE; i++)
        l1_weights[i] = *(data8++);

    shuffle_input_layer();

    NNUE_LOADED = 1;

    #endif
}

int nnue_evaluate(Thread *thread, Board *board) {

    int mg_eval, eg_eval;
    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    if (!NNUE_LOADED)
        abort_nnue("NNUE File was not provided");

    // For optimizations, auto-flag KvK as drawn
    if (kings == (white | black)) return 0;

    // Optimized computation of various input indices
    int wrelksq = relativeSquare(WHITE, getlsb(white & kings));
    int brelksq = relativeSquare(BLACK, getlsb(black & kings));

    // Large enough to handle layer computations
    ALIGN64 uint8_t out8[L1SIZE];

    NNUEAccumulator *accum = &thread->nnueStack[thread->height];

    if (!accum->accurate) {

        // Possible to recurse and incrementally update each
        if (nnue_can_update(accum, board))
            nnue_update_accumulator(accum, board, wrelksq, brelksq);

        // History is missing, we must refresh completely
        else
            nnue_refresh_accumulators(accum, board, wrelksq, brelksq);
    }

    // Feed-forward the entire evaluation function
    halfkp_relu(accum, out8, board->turn);
    int output = output_transform(l1_weights, l1_biases, out8);

    // Perform the dequantization step and upscale the Midgame
    mg_eval = 140 * (output >> SHIFT_L1) / 100;
    eg_eval = 100 * (output >> SHIFT_L1) / 100;

    // Cap the NNUE evaluation within [-2000, 2000]
    mg_eval = MAX(-2000, MIN(2000, mg_eval));
    eg_eval = MAX(-2000, MIN(2000, eg_eval));
    return MakeScore(mg_eval, eg_eval);
}
