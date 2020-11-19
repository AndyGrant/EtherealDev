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
void nnue_output_transform(float *weights, float *biases, float *inputs, float *outputs, int rows, int cols);

#if defined(_WIN32) || defined(_WIN64)

    /// Windows Support

    #include <windows.h>

    INLINE void* align_malloc(size_t size) {
        return _mm_malloc(size, 64);
    }

    INLINE void align_free(void *ptr) {
        _mm_free(ptr);
    }

#else

    /// Otherwise, assume POSIX Support

    #include <sys/time.h>

    INLINE void* align_malloc(size_t size) {
        void *mem; return posix_memalign(&mem, 64, size) ? NULL : mem;
    }

    INLINE void align_free(void *ptr) {
        free(ptr);
    }

#endif
