#pragma once

#include <stdalign.h>

#include "../types.h"

typedef struct Layer {
    int rows, cols;
    ALIGN64 float *weights;
    ALIGN64 float *biases;
} Layer;

typedef struct NNUE {
    int length;
    Layer *layers;
} NNUE;

void load_nnue(const char* fname);
int evaluate_nnue(Board *board);
void compute_nnue_indices(const Board *board, int sq, int *index1, int *index2);