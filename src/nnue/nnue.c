
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <immintrin.h>

#include "../board.h"
#include "../bitboards.h"
#include "nnue.h"


static Layer Architecture[] = {
    {40960, 256, NULL, NULL},
    {  512,  32, NULL, NULL},
    {   32,  32, NULL, NULL},
    {   32,   1, NULL, NULL},
};

NNUE nnue = (NNUE) { 4, Architecture };

void compute_nnue_indices(const Board *board, int sq, int *i1, int *i2) {

    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    const int wking = getlsb(white & kings);
    const int bking = getlsb(black & kings);

    const int stmk  = (board->turn == WHITE ? wking : bking);
    const int nstmk = (board->turn == WHITE ? bking : wking);

    const int sksq  = relativeSquare( board->turn,  stmk);
    const int nsksq = relativeSquare(!board->turn, nstmk);

    const int srelsq  = relativeSquare( board->turn, sq);
    const int nsrelsq = relativeSquare(!board->turn, sq);

    const int piece  = pieceType(board->squares[sq]);
    const int colour = pieceColour(board->squares[sq]);

    *i1 = (64 * 10 *  sksq) + (64 * (5 * (colour == board->turn) + piece)) + srelsq;
    *i2 = (64 * 10 * nsksq) + (64 * (5 * (colour != board->turn) + piece)) + nsrelsq;
}

void nnue_transform_layer(int layer);

void load_nnue(const char* fname) {

    FILE *fin = fopen(fname, "rb");

    for (int i = 0; i < nnue.length; i++) {

        const int rows = nnue.layers[i].rows;
        const int cols = nnue.layers[i].cols;

        if (nnue.layers[i].weights)
            free(nnue.layers[i].weights);
        nnue.layers[i].weights = malloc(sizeof(int16_t) * rows * cols);

        if (nnue.layers[i].biases)
            free(nnue.layers[i].biases);
        nnue.layers[i].biases = malloc(sizeof(int16_t) * cols);

        if (   fread(nnue.layers[i].biases, sizeof(int16_t), cols, fin) != (size_t) cols
            || fread(nnue.layers[i].weights, sizeof(int16_t), rows * cols, fin) != (size_t) rows * cols)
            printf("info string Unable to read NNUE\n"), fflush(stdout);
    }

    for (int layer = 1; layer < nnue.length; layer++)
        nnue_transform_layer(layer);

    fclose(fin);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

#define SHIFT 6

void nnue_input_transform(Board *board, int16_t *output);
void nnue_relu_shift(int16_t *inputs, int16_t *outputs, int length);
void nnue_affine_transform(int16_t *inputs, int16_t *outputs, int layer);

int evaluate_nnue(Board *board) {

    ALIGN64 int16_t out1_relu[2 * nnue.layers[0].cols];
    nnue_input_transform(board, out1_relu);

    ALIGN64 int16_t out2[nnue.layers[1].cols];
    ALIGN64 int16_t out2_relu[nnue.layers[1].cols];

    nnue_affine_transform(out1_relu, out2, 1);
    nnue_relu_shift(out2, out2_relu, nnue.layers[1].cols);

    ALIGN64 int16_t out3[nnue.layers[2].cols];
    ALIGN64 int16_t out3_relu[nnue.layers[2].cols];

    nnue_affine_transform(out2_relu, out3, 2);
    nnue_relu_shift(out3, out3_relu, nnue.layers[2].cols);

    ALIGN64 int16_t out4[nnue.layers[3].cols];
    nnue_affine_transform(out3_relu, out4, 3);
    return out4[0];
}

void nnue_input_transform(Board *board, int16_t *output) {

    const int cols = nnue.layers[0].cols;
    const uint64_t white = board->colours[WHITE];
    const uint64_t black = board->colours[BLACK];
    const uint64_t kings = board->pieces[KING];

    int i1, i2;
    uint64_t pieces = (white | black) & ~kings;

    int32_t temp[2 * cols];
    memset(temp, 0, sizeof(int) * 2 * cols);

    while (pieces) {

        int sq = poplsb(&pieces);
        compute_nnue_indices(board, sq, &i1, &i2);

        for (int i = 0; i < cols; i++)
            temp[i] += nnue.layers[0].weights[i1 * cols + i];

        for (int i = 0; i < cols; i++)
            temp[i + cols] += nnue.layers[0].weights[i2 * cols + i];
    }

    for (int i = 0; i < 2 * cols; i++)
        output[i] = MAX(0, (temp[i] + nnue.layers[0].biases[i % cols]) >> SHIFT);
}

void nnue_relu_shift(int16_t *inputs, int16_t *outputs, int length) {

    for (int i = 0; i < length; i++)
        outputs[i] = MAX(0, inputs[i]);
}

void nnue_affine_transform(int16_t *inputs, int16_t *outputs, int layer) {

    const int16_t *biases  = nnue.layers[layer].biases;
    const int16_t *weights = nnue.layers[layer].weights;

    const int rows = nnue.layers[layer].rows;
    const int cols = nnue.layers[layer].cols;

    for (int j = 0; j < cols; j++) {

        int32_t summation = biases[j];

        for (int i = 0; i < rows; i++)
            summation += inputs[i] * weights[j * rows + i];

        outputs[j] = summation >> SHIFT;
    }
}

void nnue_transform_layer(int layer) {

    const int rows = nnue.layers[layer].rows;
    const int cols = nnue.layers[layer].cols;

    int16_t *weights = nnue.layers[layer].weights;
    int16_t *transpose = malloc(sizeof(int16_t) * rows * cols);

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            transpose[j * rows + i] = weights[i * cols + j];

    memcpy(weights, transpose, sizeof(int16_t) * rows * cols);
}
