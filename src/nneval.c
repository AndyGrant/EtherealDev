/*
  Ethereal is a UCI chess playing engine authored by Andrew Grant.
  <https://github.com/AndyGrant/Ethereal>     <andrew@grantnet.us>

  Ethereal is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Ethereal is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "board.h"
#include "bitboards.h"
#include "evaluate.h"
#include "nneval.h"
#include "types.h"

NNCache *NNCaches[NN_EG_COUNT];
EGNetwork EGNetworks[NN_EG_COUNT];

static int evaluateRPvRP(EGNetwork *nn, NNCacheEntry *entry, Board *board);

static char *RPvRP_Weights[] = {
    #include NN_RPvRP_FILE
};


void initEndgameNNs() {

    // Allocate an NNCache for each EG network
    for (int i = 0; i < NN_EG_COUNT; i++)
        NNCaches[i] = malloc(sizeof(NNCache));

    initEndgameNN(&EGNetworks[NN_RPvRP], RPvRP_Weights, 352);

    // printEndgameNN(&EGNetworks[NN_RPvRP], "Rook(s) + Pawn(s)", 352);
}

void initEndgameNN(EGNetwork *nn, char *weights[], int inputs) {

    // Enough space for the weights and the row ptrs
    nn->inputWeights = malloc(
          sizeof(float*) * NN_EG_NEURONS
        + sizeof(float ) * NN_EG_NEURONS * inputs
    );

    // Trick to allocate a 2-dimensional array of floats
    float *ptr = (float *)(nn->inputWeights + NN_EG_NEURONS);
    for (int i = 0; i < NN_EG_NEURONS; i++)
        nn->inputWeights[i] = (ptr + inputs * i);

    // Init the weights for the the Input Layer

    for (int i = 0; i < NN_EG_NEURONS; i++) {

        char line[strlen(weights[i]) + 1];
        strcpy(line, weights[i]);
        strtok(line, " ");

        for (int j = 0; j < inputs; j++)
            nn->inputWeights[i][j] = atof(strtok(NULL, " "));
        nn->inputBiases[i] = atof(strtok(NULL, " "));
    }

    // Init the weights for the Hidden Layer 1

    for (int i = 0; i < NN_EG_NEURONS; i++) {

        char line[strlen(weights[i + NN_EG_NEURONS]) + 1];
        strcpy(line, weights[i + NN_EG_NEURONS]);
        strtok(line, " ");

        for (int j = 0; j < NN_EG_NEURONS; j++)
            nn->layer1Weights[i][j] = atof(strtok(NULL, " "));
        nn->layer1Biases[i] = atof(strtok(NULL, " "));
    }

    // Init the weights for the Hidden Layer 2

    char line[strlen(weights[2 * NN_EG_NEURONS]) + 1];
    strcpy(line, weights[2 * NN_EG_NEURONS]);
    strtok(line, " ");

    for (int i = 0; i < NN_EG_NEURONS; i++)
        nn->layer2Weights[i] = atof(strtok(NULL, " "));
    nn->layer2Bias = atof(strtok(NULL, " "));
}

void printEndgameNN(EGNetwork *nn, char *label, int inputs) {

    printf("Network Size [%d, %d, %d, 1] for %s\n", inputs, NN_EG_NEURONS, NN_EG_NEURONS, label);

    printf("Input Biases  : [ ");
    for (int i = 0; i < NN_EG_NEURONS; i++)
        printf("%+7.3f ", nn->inputBiases[i]);
    printf("]\n");

    printf("Layer1 Biases : [ ");
    for (int i = 0; i < NN_EG_NEURONS; i++)
        printf("%+7.3f ", nn->layer1Biases[i]);
    printf("]\n");

    printf("Layer2 Biases : [ %+7.3f ]\n", nn->layer2Bias);


    printf("\nInput Weights\n");
    for (int i = 0; i < NN_EG_NEURONS; i++) {
        printf("[%d] : [ ", i);
        for (int j = 0; j < 4; j++)
            printf("%+7.3f ", nn->inputWeights[i][j]);
        printf("... ");
        for (int j = inputs - 4; j < inputs; j++)
            printf("%+7.3f ", nn->inputWeights[i][j]);
        printf("]\n");
    }

    printf("\nLayer1 Weights\n");
    for (int i = 0; i < NN_EG_NEURONS; i++) {
        printf("[%d] : [ ", i);
        for (int j = 0; j < NN_EG_NEURONS; j++)
            printf("%+7.3f ", nn->layer1Weights[i][j]);
        printf("]\n");
    }

    printf("\nLayer2 Weights\n[0] : [ ");
    for (int i = 0; i < NN_EG_NEURONS; i++)
        printf("%+7.3f ", nn->layer2Weights[i]);
    printf("]\n");

    fflush(stdout);
}


int evaluateEndgames(Board *board) {

    uint64_t knights = board->pieces[KNIGHT];
    uint64_t bishops = board->pieces[BISHOP];
    uint64_t rooks   = board->pieces[ROOK  ];
    uint64_t queens  = board->pieces[QUEEN ];

    // If not an endgame (only RP at the moment), return 0
    if ((knights | bishops | queens) || !rooks)
        return MakeScore(0, 0);

    int egtype = NN_RPvRP; // Only NN we have at the moment

    NNCacheEntry *entry = &(*NNCaches[egtype])[board->pkhash & NN_CACHE_MASK];

    if (entry->key != board->pkhash)
        computeEndgameNeurons(&EGNetworks[egtype], entry, board);

    return evaluateRPvRP(&EGNetworks[egtype], entry, board);
}

void computeEndgameNeurons(EGNetwork *nn, NNCacheEntry *entry, Board *board) {

    const int PawnIDX[COLOUR_NB] = {  0,  48 };
    const int KingIDX[COLOUR_NB] = { 96, 160 };

    float neurons[NN_EG_NEURONS];
    uint64_t pawns = board->pieces[PAWN];
    uint64_t kings = board->pieces[KING];
    uint64_t black = board->colours[BLACK];

    { // Do one King first so we can set the Neuron
        int sq = poplsb(&kings);
        int idx = sq + KingIDX[testBit(black, sq)];
        for (int i = 0; i < NN_EG_NEURONS; i++)
            neurons[i] = nn->inputBiases[i] + nn->inputWeights[i][idx];
    }

    {  // Do the King that we did not do first
        int sq = poplsb(&kings);
        int idx = sq + KingIDX[testBit(black, sq)];
        for (int i = 0; i < NN_EG_NEURONS; i++)
            neurons[i] += nn->inputWeights[i][idx];
    }

    while (pawns) {
        int sq = poplsb(&pawns);
        int idx = (sq - 8) + PawnIDX[testBit(black, sq)];
        for (int i = 0; i < NN_EG_NEURONS; i++)
            neurons[i] += nn->inputWeights[i][idx];
    }

    memcpy(entry->neurons, neurons, sizeof(float) * NN_EG_NEURONS);
    entry->key = board->pkhash;
}


static int evaluateRPvRP(EGNetwork *nn, NNCacheEntry *entry, Board *board) {

    float neurons1[NN_EG_NEURONS], neurons2[NN_EG_NEURONS], output;
    memcpy(neurons1, entry->neurons, sizeof(float) * NN_EG_NEURONS);

    const int RookIDX[COLOUR_NB] = { 224, 288 };

    uint64_t rooks = board->pieces[ROOK];
    uint64_t black = board->colours[BLACK];

    // Finish computing Hidden Layer 1

    while (rooks) {
        int sq = poplsb(&rooks);
        int idx = sq + RookIDX[testBit(black, sq)];
        for (int i = 0; i < NN_EG_NEURONS; i++)
            neurons1[i] += nn->inputWeights[i][idx];
    }

    // Compute Hidden Layer 2

    for (int i = 0; i < NN_EG_NEURONS; i++) {
        neurons2[i] = nn->layer1Biases[i];
        for (int j = 0; j < NN_EG_NEURONS; j++)
            if (neurons1[j] >= 0.0)
                neurons2[i] += neurons1[j] * nn->layer1Weights[i][j];
    }

    // Compute Output Neuron for the Endgame

    output = nn->layer2Bias;
    for (int i = 0; i < NN_EG_NEURONS; i++)
        if (neurons2[i] >= 0.0)
            output += neurons2[i] * nn->layer2Weights[i];
    return MakeScore(0, (int) output);
}