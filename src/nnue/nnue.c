
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../bitboards.h"
#include "nnue.h"

#define USE_AVX2

#ifdef USE_AVX2
#include <immintrin.h>
#endif

static Layer Architecture[] = {
    {40960,  64, NULL, NULL},
    {  128,  32, NULL, NULL},
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
            free(nnue.layers[i].weights);
        nnue.layers[i].weights = malloc(sizeof(float) * rows * cols);

        if (nnue.layers[i].biases)
            free(nnue.layers[i].biases);
        nnue.layers[i].biases = malloc(sizeof(float) * cols);

        if (   fread(nnue.layers[i].biases, sizeof(float), cols, fin) != (size_t) cols
            || fread(nnue.layers[i].weights, sizeof(float), rows * cols, fin) != (size_t) rows * cols)
            printf("info string Unable to read NNUE\n"), fflush(stdout);
    }

    fclose(fin);
}

int evaluate_nnue(Board *board) {

    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    const int cols = nnue.layers[0].cols;

    ALIGN64 float out1[2 * cols], out1_relu[2*cols];
    ALIGN64 float out2[nnue.layers[1].cols], out2_relu[nnue.layers[1].cols];
    ALIGN64 float out3[nnue.layers[2].cols], out3_relu[nnue.layers[2].cols];
    ALIGN64 float out4[nnue.layers[3].cols];

    memcpy(out1, nnue.layers[0].biases, sizeof(float) * cols);
    memcpy(out1 + cols, nnue.layers[0].biases, sizeof(float) * cols);

    int i1, i2;
    uint64_t pieces = (white | black) & ~kings;

    while (pieces) {

        int sq = poplsb(&pieces);
        compute_nnue_indices(board, sq, &i1, &i2);

        const int woffset = board->turn == WHITE ? 0 : cols;
        const int boffset = board->turn == WHITE ? cols : 0;

        for (int i = 0; i < cols; i++)
            out1[i+woffset] += nnue.layers[0].weights[i1 * cols + i];

        for (int i = 0; i < cols; i++)
            out1[i+boffset] += nnue.layers[0].weights[i2 * cols + i];
    }

    nnue_relu(out1, out1_relu, nnue.layers[1].rows);
    nnue_affine_transform(nnue.layers[1].weights, nnue.layers[1].biases,
        out1_relu, out2, nnue.layers[1].rows, nnue.layers[1].cols);


    nnue_relu(out2, out2_relu, nnue.layers[2].rows);
    nnue_affine_transform(nnue.layers[2].weights, nnue.layers[2].biases,
        out2_relu, out3, nnue.layers[2].rows, nnue.layers[2].cols);


    nnue_relu(out3, out3_relu, nnue.layers[3].rows);
    nnue_affine_transform(nnue.layers[3].weights, nnue.layers[3].biases,
        out3_relu, out4, nnue.layers[3].rows, nnue.layers[3].cols);


    return out4[0];
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

#ifdef USE_AVX2

    const __m256 zero = _mm256_setzero_ps();

    __m256 *in  = (__m256 *) inputs;
    __m256 *out = (__m256 *) outputs;

    for (int i = 0; i < length / 8; i++)
        out[i] = _mm256_max_ps(zero, in[i]);

#else

    for (int i = 0; i < length; i++)
        outputs[i] = fmaxf(0.0f, inputs[i]);

#endif

}

void nnue_affine_transform(float *weights, float *biases, float *inputs, float *outputs, int rows, int cols) {

    if (cols == 1) {

        {
            for (int j = 0; j < cols; j++)
                outputs[j] = biases[j] + inputs[0] * weights[j];
        }

        for (int i = 1; i < rows; i++)
            for (int j = 0; j < cols; j++)
                outputs[j] += inputs[i] * weights[i * cols + j];

        return ;
    }

    __m256 result;
    __m256 *out = (__m256 *) outputs;
    __m256 *bia = (__m256 *) biases;

    {
        __m256 in1 = _mm256_set1_ps(inputs[0]);
        __m256 in2 = _mm256_set1_ps(inputs[1]);
        __m256 in3 = _mm256_set1_ps(inputs[2]);
        __m256 in4 = _mm256_set1_ps(inputs[3]);

        for (int j = 0; j < cols / 8; j++) {

            __m256 w1 = _mm256_loadu_ps(&weights[0 * cols + 8 * j]);
            __m256 w2 = _mm256_loadu_ps(&weights[1 * cols + 8 * j]);
            __m256 w3 = _mm256_loadu_ps(&weights[2 * cols + 8 * j]);
            __m256 w4 = _mm256_loadu_ps(&weights[3 * cols + 8 * j]);

            result = _mm256_fmadd_ps(in1, w1, bia[j]);
            result = _mm256_fmadd_ps(in2, w2, result);
            result = _mm256_fmadd_ps(in3, w3, result);
            out[j] = _mm256_fmadd_ps(in4, w4, result);
        }
    }

    for (int i = 4; i < rows; i += 4) {

        __m256 in1 = _mm256_set1_ps(inputs[i+0]);
        __m256 in2 = _mm256_set1_ps(inputs[i+1]);
        __m256 in3 = _mm256_set1_ps(inputs[i+2]);
        __m256 in4 = _mm256_set1_ps(inputs[i+3]);

        for (int j = 0; j < cols / 8; j++) {

            __m256 w1 = _mm256_loadu_ps(&weights[(i+0) * cols + 8 * j]);
            __m256 w2 = _mm256_loadu_ps(&weights[(i+1) * cols + 8 * j]);
            __m256 w3 = _mm256_loadu_ps(&weights[(i+2) * cols + 8 * j]);
            __m256 w4 = _mm256_loadu_ps(&weights[(i+3) * cols + 8 * j]);

            result = _mm256_fmadd_ps(in1, w1, out[j]);
            result = _mm256_fmadd_ps(in2, w2, result);
            result = _mm256_fmadd_ps(in3, w3, result);
            out[j] = _mm256_fmadd_ps(in4, w4, result);
        }
    }
}
