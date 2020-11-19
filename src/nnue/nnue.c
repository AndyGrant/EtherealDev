
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../bitboards.h"
#include "nnue.h"

#include <immintrin.h>

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
        outN_relu, outN, nnue.layers[1].rows, 32); // nnue.layers[1].cols);

    nnue_relu(outN, outN_relu, nnue.layers[2].rows);
    nnue_affine_transform(nnue.layers[2].weights, nnue.layers[2].biases,
        outN_relu, outN, nnue.layers[2].rows, 32); // nnue.layers[2].cols);

    nnue_relu(outN, outN_relu, nnue.layers[3].rows);
    nnue_output_transform(nnue.layers[3].weights, nnue.layers[3].biases,
        outN_relu, outN, nnue.layers[3].rows, nnue.layers[3].cols);

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

void nnue_affine_transform(float *weights, float *biases, float *inputs, float *outputs, int rows, int cols) {

    __m256 results[cols / 8];
    for (int j = 0; j < cols / 8; j++)
        results[j] = _mm256_load_ps(&biases[j*8]);

    for (int i = 0; i < rows; i += 4) {

        __m256 in1 = _mm256_set1_ps(inputs[i+0]);
        __m256 in2 = _mm256_set1_ps(inputs[i+1]);
        __m256 in3 = _mm256_set1_ps(inputs[i+2]);
        __m256 in4 = _mm256_set1_ps(inputs[i+3]);

        for (int j = 0; j < cols / 8; j++) {

            __m256 w1 = _mm256_load_ps(&weights[(i+0) * cols + 8 * j]);
            __m256 w2 = _mm256_load_ps(&weights[(i+1) * cols + 8 * j]);
            __m256 w3 = _mm256_load_ps(&weights[(i+2) * cols + 8 * j]);
            __m256 w4 = _mm256_load_ps(&weights[(i+3) * cols + 8 * j]);

            results[j] = _mm256_fmadd_ps(in1, w1, results[j]);
            results[j] = _mm256_fmadd_ps(in2, w2, results[j]);
            results[j] = _mm256_fmadd_ps(in3, w3, results[j]);
            results[j] = _mm256_fmadd_ps(in4, w4, results[j]);
        }
    }

    for (int j = 0; j < cols / 8; j++)
        _mm256_store_ps(&outputs[j*8], results[j]);
}

void nnue_output_transform(float *weights, float *biases, float *inputs, float *outputs, int rows, int cols) {

    {
        for (int j = 0; j < cols; j++)
            outputs[j] = biases[j] + inputs[0] * weights[j];
    }

    for (int i = 1; i < rows; i++)
        for (int j = 0; j < cols; j++)
            outputs[j] += inputs[i] * weights[i * cols + j];
}