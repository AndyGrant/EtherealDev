
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../board.h"
#include "../bitboards.h"
#include "nnue.h"

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

    float out1[2 * cols];
    float out2[nnue.layers[1].cols];
    float out3[nnue.layers[2].cols];
    float out4[nnue.layers[3].cols];

    memcpy(out1, nnue.layers[0].biases, sizeof(float) * cols);
    memcpy(out1 + cols, nnue.layers[0].biases, sizeof(float) * cols);

    memcpy(out2, nnue.layers[1].biases, sizeof(float) * nnue.layers[1].cols);
    memcpy(out3, nnue.layers[2].biases, sizeof(float) * nnue.layers[2].cols);
    memcpy(out4, nnue.layers[3].biases, sizeof(float) * nnue.layers[3].cols);

    int index1, index2;
    uint64_t pieces = (white | black) & ~kings;

    while (pieces) {

        int sq = poplsb(&pieces);
        compute_nnue_indices(board, sq, &index1, &index2);

        for (int i = 0; i < cols; i++)
            out1[i] += nnue.layers[0].weights[index1 * cols + i];

        for (int i = 0; i < cols; i++)
            out1[i + cols] += nnue.layers[0].weights[index2 * cols + i];
    }

    for (int i = 0; i < nnue.layers[1].rows; i++)
        for (int j = 0; j < nnue.layers[1].cols; j++)
            out2[j] += fmaxf(0.0f, out1[i]) * nnue.layers[1].weights[i * nnue.layers[1].cols + j];

    for (int i = 0; i < nnue.layers[2].rows; i++)
        for (int j = 0; j < nnue.layers[2].cols; j++)
            out3[j] += fmaxf(0.0f, out2[i]) * nnue.layers[2].weights[i * nnue.layers[2].cols + j];

    for (int i = 0; i < nnue.layers[3].rows; i++)
        for (int j = 0; j < nnue.layers[3].cols; j++)
            out4[j] += fmaxf(0.0f, out3[i]) * nnue.layers[3].weights[i * nnue.layers[3].cols + j];

    return out4[0];
}

void compute_nnue_indices(const Board *board, int sq, int *index1, int *index2) {

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

    *index1 = (64 * 10 *  sksq) + (64 * (5 * (colour == board->turn) + piece)) + srelsq;
    *index2 = (64 * 10 * nsksq) + (64 * (5 * (colour != board->turn) + piece)) + nsrelsq;
}