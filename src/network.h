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

#pragma once

#include <stdint.h>

#include "board.h"
#include "types.h"


#define PKNETWORK_INPUTS  (224)
#define PKNETWORK_LAYER1  ( 32)
#define PKNETWORK_OUTPUTS (  2)

typedef struct PKNetwork {

    // PKNetworks are of the form [Input, Hidden Layer 1, Output Layer]
    // Our current Network is [224x32, 32x1]. The Network is trained to
    // output a Score in CentiPawns for the Midgame and Endgame

    ALIGN64 float inputWeights[PKNETWORK_LAYER1][PKNETWORK_INPUTS];
    ALIGN64 float inputBiases[PKNETWORK_LAYER1];

    ALIGN64 float layer1Weights[PKNETWORK_OUTPUTS][PKNETWORK_LAYER1];
    ALIGN64 float layer1Biases[PKNETWORK_OUTPUTS];

} PKNetwork;

void initPKNetwork();
int fullyComputePKNetwork(Thread *thread);
int partiallyComputePKNetwork(Thread *thread);

void initPKNetworkCollector(Thread *thread);
void updatePKNetworkIndices(Thread *thread, int changes, int indexes[3], int signs[3]);
void updatePKNetworkAfterMove(Thread *thread, uint16_t move);


#define RPKvRPK_NETWORK_INPUTS  (352)
#define RPKvRPK_NETWORK_LAYER1  ( 32)
#define RPKvRPK_NETWORK_OUTPUTS (  2)

typedef struct RPKvRPK_Network {

    // RPKvRPK Networks are of the form [Input, Hidden Layer 1, Output]
    // Our current Network is [352x32, 32x2]. The Network is trained
    // to output a score in CentiPawns for the Midgame and Endgame, when there
    // are Rook(s) on the board, with no other non-pawn material.

    ALIGN64 float inputWeights[RPKvRPK_NETWORK_LAYER1][RPKvRPK_NETWORK_INPUTS];
    ALIGN64 float inputBiases[RPKvRPK_NETWORK_LAYER1];

    ALIGN64 float layer1Weights[RPKvRPK_NETWORK_OUTPUTS][RPKvRPK_NETWORK_LAYER1];
    ALIGN64 float layer1Biases[RPKvRPK_NETWORK_OUTPUTS];

} RPKvRPK_Network;

void initRPKvRPK_Network();
int fullyComputeRPKvRPK_Network();