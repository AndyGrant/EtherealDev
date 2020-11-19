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
void compute_nnue_indices(const Board *board, int sq, int *i1, int *i2);

void nnue_relu(float *inputs, float *outputs, int length);
void nnue_affine_transform(float *weights, float *biases, float *inputs, float *outputs, int rows, int cols);
void nnue_transform_layer(int layer);