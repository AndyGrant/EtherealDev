#include <sstream>
#include <cstring>

#include "network.h"

#include "types.h"
#include "board.h"
#include "bitboards.h"

static const char *WeightsTXT[] = {
    #include "net_32x_static.net"
    ""
};

Network InitNetwork()
{
    std::stringstream stream;

    for (int i = 0; strcmp(WeightsTXT[i], ""); i++)
        stream << WeightsTXT[i] << "\n";

    std::string line;

    std::vector<std::vector<float>> weights;
    std::vector<size_t> LayerNeurons;
    weights.push_back({});

    while (getline(stream, line))
    {
        std::istringstream iss(line);
        std::string token;

        iss >> token;

        if (token == "InputNeurons")
        {
            getline(stream, line);
            std::istringstream lineStream(line);
            lineStream >> token;
            LayerNeurons.push_back(stoull(token));
        }

        if (token == "HiddenLayerNeurons")
        {
            std::vector<float> layerWeights;

            iss >> token;
            size_t num = stoull(token);

            LayerNeurons.push_back(num);

            for (size_t i = 0; i < num; i++)
            {
                getline(stream, line);
                std::istringstream lineStream(line);

                while (lineStream >> token)
                {
                    layerWeights.push_back(stof(token));
                }
            }

            weights.push_back(layerWeights);
        }

        else if (token == "OutputLayer")
        {
            LayerNeurons.push_back(1);  //always 1 output neuron
            std::vector<float> layerWeights;
            getline(stream, line);
            std::istringstream lineStream(line);
            while (lineStream >> token)
            {
                layerWeights.push_back(stof(token));
            }
            weights.push_back(layerWeights);
        }
    }

    return Network(weights, LayerNeurons);
}


Neuron::Neuron(const std::vector<float>& Weight, float Bias) : weights(Weight), bias(Bias) {}


float Neuron::FeedForward(std::vector<float>& input) const {

    assert(input.size() == weights.size());

    float ret = bias;

    for (size_t i = 0; i < input.size(); i++)
        ret += std::max(0.f, input[i]) * weights[i];

    return ret;
}


HiddenLayer::HiddenLayer(std::vector<float> inputs, size_t NeuronCount)
{
    assert(inputs.size() % NeuronCount == 0);

    size_t WeightsPerNeuron = inputs.size() / NeuronCount;

    for (size_t i = 0; i < NeuronCount; i++)
    {
        neurons.push_back(Neuron(std::vector<float>(inputs.begin() + (WeightsPerNeuron * i), inputs.begin() + (WeightsPerNeuron * i) + WeightsPerNeuron - 1), inputs.at(WeightsPerNeuron * (1 + i) - 1)));
    }

    for (size_t i = 0; i < WeightsPerNeuron - 1; i++)
    {
        for (size_t j = 0; j < NeuronCount; j++)
        {
            weightTranspose.push_back(neurons.at(j).weights.at(i));
        }
    }

    zeta = std::vector<float>(NeuronCount, 0);
}


std::vector<float> HiddenLayer::FeedForward(std::vector<float>& input)
{
    for (size_t i = 0; i < neurons.size(); i++)
    {
        zeta[i] = neurons.at(i).FeedForward(input);
    }

    return zeta;
}


void HiddenLayer::ApplyDelta(std::vector<deltaPoint>& deltaVec, float forward)
{
    size_t neuronCount = zeta.size();
    size_t deltaCount = deltaVec.size();

    for (size_t index = 0; index < deltaCount; index++)
    {
        float deltaValue = deltaVec[index].delta * forward;
        size_t weightTransposeIndex = deltaVec[index].index * neuronCount;

        for (size_t neuron = 0; neuron < neuronCount; neuron++)
        {
            zeta[neuron] += weightTranspose[weightTransposeIndex + neuron] * deltaValue;
        }
    }
}


Network::Network(std::vector<std::vector<float>> inputs, std::vector<size_t> NeuronCount)
    : outputNeuron(std::vector<float>(inputs.back().begin(), inputs.back().end() - 1), inputs.back().back())
{
    assert(inputs.size() == NeuronCount.size());

    zeta = 0;
    inputNeurons = NeuronCount.at(0);

    for (size_t i = 1; i < NeuronCount.size() - 1; i++)
    {
        hiddenLayers.push_back(HiddenLayer(inputs.at(i), NeuronCount.at(i)));
    }
}


float Network::FeedForward(std::vector<float> inputs) {

    assert(inputs.size() == inputNeurons);

    for (size_t i = 0; i < hiddenLayers.size(); i++)
        inputs = hiddenLayers.at(i).FeedForward(inputs);

    return outputNeuron.FeedForward(inputs);
}


void Network::ApplyDelta(std::vector<deltaPoint>& delta)
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ApplyDelta(delta, 1);
}


void Network::ApplyInverseDelta(std::vector<deltaPoint>& delta)
{
    assert(hiddenLayers.size() > 0);
    hiddenLayers[0].ApplyDelta(delta, -1);
}

Network CreateRandom(std::vector<size_t> NeuronCount)
{
    std::random_device rd;
    std::mt19937 e2(rd());

    std::vector<std::vector<float>> inputs;
    inputs.push_back({});

    size_t prevLayerNeurons = NeuronCount[0];

    for (size_t layer = 1; layer < NeuronCount.size() - 1; layer++)
    {
        //see https://arxiv.org/pdf/1502.01852v1.pdf eq (10) for theoretical significance of w ~ N(0, sqrt(2/n))
        std::normal_distribution<> dist(0.0, sqrt(2.0 / static_cast<float>(NeuronCount[layer])));

        std::vector<float> input;
        for (size_t i = 0; i < (prevLayerNeurons + 1) * NeuronCount[layer]; i++)
        {
            if ((i + 1) % (prevLayerNeurons + 1) == 0)
                input.push_back(0);
            else
                input.push_back(static_cast<float>(dist(e2)));
        }
        inputs.push_back(input);
        prevLayerNeurons = NeuronCount[layer];
    }

    std::normal_distribution<> dist(0.0, sqrt(2.0 / static_cast<float>(NeuronCount.back())));

    //connections from last hidden to output
    std::vector<float> input;
    for (size_t i = 0; i < (prevLayerNeurons + 1) * NeuronCount.back(); i++)
    {
        if ((i + 1) % (prevLayerNeurons + 1) == 0)
            input.push_back(0);
        else
            input.push_back(static_cast<float>(dist(e2)));
    }
    inputs.push_back(input);

    return Network(inputs, NeuronCount);
}




int Network::evaluate(Board *board) {

    std::vector<float> inputs;
    inputs.reserve(224);

    for (int colour = WHITE; colour <= BLACK; colour++) {

        for (int sq = 0; sq < SQUARE_NB; sq++)
            inputs.push_back(testBit(board->colours[colour] & board->pieces[PAWN], sq));

        for (int sq = 0; sq < SQUARE_NB; sq++)
            if (!testBit(PROMOTION_RANKS, sq))
                inputs.push_back(testBit(board->colours[colour] & board->pieces[KING], sq));
    }

    int X = FeedForward(inputs);
    return 0;
    return X;
}