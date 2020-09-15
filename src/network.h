#pragma once

#include <array>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <assert.h>
#include <random>
#include <numeric>
#include <algorithm>
#include <sstream>

#include "types.h"

#define INPUT_NEURONS (64 + 48 + 64 + 48)


struct deltaPoint
{
    size_t index;
    int delta;
};

struct Neuron
{
    Neuron(const std::vector<float>& Weight, float Bias);
    float FeedForward(std::vector<float>& input) const;

    std::vector<float> weights;
    float bias;
};

struct HiddenLayer
{
    HiddenLayer(std::vector<float> inputs, size_t NeuronCount);    // <for first neuron>: weight1, weight2, ..., weightN, bias, <next neuron etc...>
    std::vector<float> FeedForward(std::vector<float>& input);

    std::vector<Neuron> neurons;

    //cache for backprop after feedforward
    std::vector<float> zeta;    //weighted input

    void ApplyDelta(std::vector<deltaPoint>& deltaVec, float forward);            //incrementally update the connections between input layer and first hidden layer

private:

    std::vector<float> weightTranspose; //first neuron first weight, second neuron first weight etc...
};

struct Network
{
    Network(std::vector<std::vector<float>> inputs, std::vector<size_t> NeuronCount);
    float FeedForward(std::vector<float> inputs);
    void ApplyDelta(std::vector<deltaPoint>& delta);            //incrementally update the connections between input layer and first hidden layer
    void ApplyInverseDelta(std::vector<deltaPoint>& delta);     //for un-make moves
    int evaluate(Board *board);


private:
    size_t inputNeurons;

    //cache for backprop after feedforward (these are for the output neuron)
    float zeta;    //weighted input

    std::vector<HiddenLayer> hiddenLayers;
    Neuron outputNeuron;
};

Network InitNetwork(std::string file);
Network CreateRandom(std::vector<size_t> NeuronCount);