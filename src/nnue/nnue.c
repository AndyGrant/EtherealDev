
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../bitboards.h"
#include "nnue.h"

#include <immintrin.h>

// 2 x (40960 x N) => 2N x 32 x 32 x 1 Assumed

Layer Architecture[] = {
    {40960, 256, NULL, NULL},
    {  512,  32, NULL, NULL},
    {   32,  32, NULL, NULL},
    {   32,   1, NULL, NULL},
};

NNUE nnue = (NNUE) { 4, Architecture };


void load_nnue(const char* fname) {

    FILE *fin = fopen(fname, "rb");

    for (int i = 0; i < nnue.length; i++) {

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

int evaluate_nnue(Board *board) {

    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    const int cols = nnue.layers[0].cols;

    // Large enough to handle all local operations
    ALIGN64 float outN[2 * cols], outN_relu[2 * cols];

    memcpy(outN, nnue.layers[0].biases, sizeof(float) * cols);
    memcpy(outN + cols, nnue.layers[0].biases, sizeof(float) * cols);

    int i1, i2;
    uint64_t pieces = (white | black) & ~kings;

    while (pieces) {

        int sq = poplsb(&pieces);
        compute_nnue_indices(board, sq, &i1, &i2);

        const int woffset = board->turn == WHITE ? 0 : cols;
        const int boffset = board->turn == WHITE ? cols : 0;

        for (int i = 0; i < cols; i++)
            outN[i+woffset] += nnue.layers[0].weights[i1 * cols + i];

        for (int i = 0; i < cols; i++)
            outN[i+boffset] += nnue.layers[0].weights[i2 * cols + i];
    }

    nnue_relu(outN, outN_relu, nnue.layers[1].rows);
    nnue_affine_transform(nnue.layers[1].weights, nnue.layers[1].biases,
        outN_relu, outN, nnue.layers[1].rows); // nnue.layers[1].cols);

    nnue_relu(outN, outN_relu, nnue.layers[2].rows);
    nnue_affine_transform(nnue.layers[2].weights, nnue.layers[2].biases,
        outN_relu, outN, nnue.layers[2].rows); // nnue.layers[2].cols);

    nnue_relu(outN, outN_relu, nnue.layers[3].rows);
    nnue_output_transform(nnue.layers[3].weights, nnue.layers[3].biases,
        outN_relu, outN, nnue.layers[3].rows); // nnue.layers[3].cols

    return outN[0];
}

void compute_nnue_indices(const Board *board, int sq, int *i1, int *i2) {

    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    const int wksq = relativeSquare(WHITE, getlsb(white & kings));
    const int bksq = relativeSquare(BLACK, getlsb(black & kings));

    const int wrelsq = relativeSquare(WHITE, sq);
    const int brelsq = relativeSquare(BLACK, sq);

    const int piece  = pieceType(board->squares[sq]);
    const int colour = pieceColour(board->squares[sq]);

    *i1 = (64 * 10 * wksq) + (64 * (5 * (colour == WHITE) + piece)) + wrelsq;
    *i2 = (64 * 10 * bksq) + (64 * (5 * (colour == BLACK) + piece)) + brelsq;
}


void nnue_relu(float *inputs, float *outputs, int length) {

    const __m256 zero = _mm256_setzero_ps();

    __m256 *in  = (__m256 *) inputs;
    __m256 *out = (__m256 *) outputs;

    for (int i = 0; i < length / 8; i += 4) {
        out[i+0] = _mm256_max_ps(zero, in[i+0]);
        out[i+1] = _mm256_max_ps(zero, in[i+1]);
        out[i+2] = _mm256_max_ps(zero, in[i+2]);
        out[i+3] = _mm256_max_ps(zero, in[i+3]);
    }
}

void nnue_affine_transform(float *weights, float *biases, float *inputs, float *outputs, int rows) {

    /// Solution Derived from https://github.com/Luecx/Koivisto

    const int InChunks  = rows / 8;
    const int OutChunks = 32   / 8;

    const __m256 *inp = (__m256 *) inputs;
    const __m256 *bia = (__m256 *) biases;
    const __m256 *wgt = (__m256 *) weights;
    __m256 *const out = (__m256 *) outputs;

    for (int i = 0; i < OutChunks; i++) {

        __m256 acc0 = _mm256_setzero_ps();
        __m256 acc1 = _mm256_setzero_ps();
        __m256 acc2 = _mm256_setzero_ps();
        __m256 acc3 = _mm256_setzero_ps();
        __m256 acc4 = _mm256_setzero_ps();
        __m256 acc5 = _mm256_setzero_ps();
        __m256 acc6 = _mm256_setzero_ps();
        __m256 acc7 = _mm256_setzero_ps();

        for (int j = 0; j < InChunks; j++) {
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
        out[i] = _mm256_add_ps(bia[i], acc0);
    }
}

void nnue_output_transform(float *weights, float *biases, float *inputs, float *outputs, int rows) {

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

void nnue_transpose(float *matrix, int rows, int cols) {

    float *cpy = malloc(sizeof(float) * rows * cols);

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            cpy[j * rows + i] = matrix[i * cols + j];

    memcpy(matrix, cpy, sizeof(float) * rows * cols);
    free(cpy);
}