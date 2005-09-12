/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC.
*/


#include "fl/neural.h"
#include "fl/math.h"


using namespace std;
using namespace fl;


// class NeuralNetworkBackprop ------------------------------------------------

NeuralNetworkBackprop::~NeuralNetworkBackprop ()
{
  destroyNetwork ();
}

void
NeuralNetworkBackprop::destroyNetwork ()
{
  for (int i = 0; i < outputs.size (); i++)
  {
	delete outputs[i];
  }
  for (int i = 0; i < hidden.size (); i++)
  {
	delete hidden[i];
  }
  for (int i = 0; i < inputs.size (); i++)
  {
	delete inputs[i];
  }

  inputs.clear ();
  hidden.clear ();
  outputs.clear ();
}

void
NeuralNetworkBackprop::constructHiddenLayer (int n)
{
  vector<int> tempSizes;
  tempSizes.push_back (n);
  constructHiddenLayers (tempSizes);
}

void
NeuralNetworkBackprop::constructHiddenLayers (vector<int> & sizes)
{
  if (sizes.size () < 1)
  {
	// Directly connect the inputs and outputs (no hidden layer).
	for (int i = 0; i < inputs.size (); i++)
	{
	  for (int j = 0; j < outputs.size (); j++)
	  {
		new SynapseBackprop (inputs[i], outputs[j]);
	  }
	}
	return;
  }

  // Inputs to first hidden layer
  for (int i = 0; i < sizes[0]; i++)
  {
	NeuronBackprop * newNeuron = new NeuronBackprop;
	hidden.push_back (newNeuron);

	for (int j = 0; j < inputs.size (); j++)
	{
	  new SynapseBackprop (inputs[j], newNeuron);
	}
  }

  // Hidden layer to hidden layer
  for (int i = 1; i < sizes.size (); i++)
  {
	int h = hidden.size () - sizes[i-1];
	for (int j = 0; j < sizes[i]; j++)
	{
	  NeuronBackprop * newNeuron = new NeuronBackprop;
	  hidden.push_back (newNeuron);

	  for (int k = 0; k < sizes[i-1]; k++)
	  {
		new SynapseBackprop (hidden[h+k], newNeuron);
	  }
	}
  }

  // Hidden layer to output layer
  int h = hidden.size () - sizes.back ();
  for (; h < hidden.size (); h++)
  {
	for (int j = 0; j < outputs.size (); j++)
	{
	  new SynapseBackprop (hidden[h], outputs[j]);
	}
  }
}

void
NeuralNetworkBackprop::train (float tolerance)
{
  int iteration = 0;
  int stable = 0;
  float smallestError = INFINITY;
  while (stable < 3)
  {
	// Step thru data
	int correctCount = 0;
	int dataCount = 0;
	float error = 0;
	startData ();
	while (nextDatum ())
	{
	  dataCount++;
	  reset ();

	  if (correct ())
	  {
		correctCount++;
	  }

	  // backprop
	  for (int i = 0; i < outputs.size (); i++)
	  {
		outputs[i]->learn ();

		// accumulate error for convergence test
		float d = outputs[i]->getDelta ();  // For an output neuron, delta should be difference between desired and actual value.
		error += d * d;
	  }		
	  for (int i = 0; i < hidden.size (); i++)
	  {
		hidden[i]->learn ();
	  }
	  for (int i = 0; i < inputs.size (); i++)
	  {
		inputs[i]->learn ();
	  }
	}
	error = sqrtf (error / dataCount);

	happyGraph (iteration++, (float) correctCount / dataCount);

	// Check for convergence
	float improvement = smallestError - error;
	if (improvement > 0)
	{
	  smallestError = error;
	}
	if (fabsf (improvement) > tolerance)
	{
	  stable = 0;
	}
	else
	{
	  stable++;
	}
	cerr << "   " << error << " " << improvement << " " << stable << endl;
  }
}

bool
NeuralNetworkBackprop::correct ()
{
  return false;
}

void
NeuralNetworkBackprop::happyGraph (int iteration, float accuracy)
{
  // do nothing
}

void
NeuralNetworkBackprop::reset ()
{
  for (int i = 0; i < inputs.size (); i++)
  {
	inputs[i]->startCycle ();
  }
  for (int i = 0; i < hidden.size (); i++)
  {
	hidden[i]->startCycle ();
  }
  for (int i = 0; i < outputs.size (); i++)
  {
	outputs[i]->startCycle ();
  }
}


// class NeuronBackprop -------------------------------------------------------

NeuronBackprop::NeuronBackprop ()
{
  new SynapseBias (this);
}

void
NeuronBackprop::startCycle ()
{
  activation = NAN;
  delta = NAN;
}

float
NeuronBackprop::getActivation ()
{
  if (isnan (activation))
  {
	activation = 0;
	vector<Synapse *>::iterator i;
	for (i = inputs.begin (); i < inputs.end (); i++)
	{
	  activation += ((SynapseBackprop *) (*i))->getOutput ();
	}
  }

  return activation;
}

float
NeuronBackprop::getOutput ()
{
  return tanh (getActivation ());
}

float
NeuronBackprop::getDelta ()
{
  if (isnan (delta))
  {
	delta = 0;
	vector<Synapse *>::iterator i;
	for (i = outputs.begin (); i < outputs.end (); i++)
	{
	  delta += ((SynapseBackprop *) (*i))->getError ();
	}
  }

  return delta;
}

float
NeuronBackprop::getError ()
{
  float o = getOutput ();
  return getDelta () * (1 - o * o);
}

void
NeuronBackprop::learn ()
{
  vector<Synapse *>::iterator i;
  for (i = inputs.begin (); i < inputs.end (); i++)
  {
	((SynapseBackprop *) (*i))->learn ();
  }
}


// class NeuronDelay ----------------------------------------------------------

NeuronDelay::NeuronDelay ()
{
  activation = 0;
  lastActivation = 0;

  // Throw away SynapseBias
  delete *(inputs.begin ());
  inputs.clear ();
}

void
NeuronDelay::startCycle ()
{
  lastActivation = activation;
  NeuronBackprop::startCycle ();
}

float
NeuronDelay::getOutput ()
{
  if (inputs.size () > 0  &&  ((SynapseBackprop *) inputs[0])->isActivationValid ())
  {
	getActivation ();
  }
  return lastActivation;
}

float
NeuronDelay::getDelta ()
{
  return 0;
}


// class SynapseBackprop ------------------------------------------------------

float SynapseBackprop::eta = 0.1f;
float SynapseBackprop::largestChange;

float
SynapseBackprop::getError ()
{
  return ((NeuronBackprop *) to)->getError () * weight;
}

float
SynapseBackprop::getOutput ()
{
  return ((NeuronBackprop *) from)->getOutput () * weight;
}

void
SynapseBackprop::learn ()
{
  float change = eta * ((NeuronBackprop *) to)->getError () * ((NeuronBackprop *) from)->getOutput ();
  weight += change;

  largestChange = max (largestChange, change);
}

bool
SynapseBackprop::isActivationValid ()
{
  return from  &&  ! isnan (((NeuronBackprop *) from)->activation);
}


// class SynapseBias ---------------------------------------------------------

float
SynapseBias::getOutput ()
{
  return weight;
}

void
SynapseBias::learn ()
{
  float change = eta * ((NeuronBackprop *) to)->getError ();
  weight += change;

  largestChange = max (largestChange, change);
}
