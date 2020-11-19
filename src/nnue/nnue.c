
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../bitboards.h"
#include "nnue.h"

#include <immintrin.h>

static Layer Architecture[] = {
    { 40960, 256 },
    {   512,  32 },
    {    32,  32 },
    {    32,   1 },
};

NNUE nnue = (NNUE) { 4, Architecture };


ALIGN64 float in_weights[40960 * 256];
ALIGN64 float in_biases[256];

ALIGN64 float l1_weights[512 * 32];
ALIGN64 float l1_biases[32];

ALIGN64 float l2_weights[32 * 32];
ALIGN64 float l2_biases[32];

ALIGN64 float l3_weights[32 * 1];
ALIGN64 float l3_biases[1];


void load_nnue(const char* fname) {

    FILE *fin = fopen(fname, "rb");

    fread(in_biases,  sizeof(float), sizeof(in_biases ) / sizeof(float), fin);
    fread(in_weights, sizeof(float), sizeof(in_weights) / sizeof(float), fin);

    fread(l1_biases,  sizeof(float), sizeof(l1_biases ) / sizeof(float), fin);
    fread(l1_weights, sizeof(float), sizeof(l1_weights) / sizeof(float), fin);

    fread(l2_biases,  sizeof(float), sizeof(l2_biases ) / sizeof(float), fin);
    fread(l2_weights, sizeof(float), sizeof(l2_weights) / sizeof(float), fin);

    fread(l3_biases,  sizeof(float), sizeof(l3_biases ) / sizeof(float), fin);
    fread(l3_weights, sizeof(float), sizeof(l3_weights) / sizeof(float), fin);

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

    memcpy(out1, in_biases, sizeof(float) * cols);
    memcpy(out1 + cols, in_biases, sizeof(float) * cols);

    int i1, i2;
    uint64_t pieces = (white | black) & ~kings;

    while (pieces) {

        int sq = poplsb(&pieces);
        compute_nnue_indices(board, sq, &i1, &i2);

        const int woffset = board->turn == WHITE ? 0 : cols;
        const int boffset = board->turn == WHITE ? cols : 0;

        for (int i = 0; i < cols; i++)
            out1[i+woffset] += in_weights[i1 * cols + i];

        for (int i = 0; i < cols; i++)
            out1[i+boffset] += in_weights[i2 * cols + i];
    }

    nnue_relu(out1, out1_relu, nnue.layers[1].rows);
    nnue_affine_transform(l1_weights, l1_biases, out1_relu, out2, nnue.layers[1].rows, nnue.layers[1].cols);

    nnue_relu(out2, out2_relu, nnue.layers[2].rows);
    nnue_affine_transform(l2_weights, l2_biases, out2_relu, out3, nnue.layers[2].rows, nnue.layers[2].cols);

    nnue_relu(out3, out3_relu, nnue.layers[3].rows);
    nnue_output_transform(l3_weights, l3_biases, out3_relu, out4, nnue.layers[3].rows, nnue.layers[3].cols);

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

    const __m256 zero = _mm256_setzero_ps();

    __m256 *in  = (__m256 *) inputs;
    __m256 *out = (__m256 *) outputs;

    for (int i = 0; i < length / 8; i++)
        out[i] = _mm256_max_ps(zero, in[i]);
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