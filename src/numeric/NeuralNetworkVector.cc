/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/neural.h"

#include <math.h>


using namespace std;
using namespace fl;


// class NeuralNetworkVector --------------------------------------------------

NeuralNetworkVector::NeuralNetworkVector (int inputSize, int outputSize, int hiddenSize)
{
  vector<int> tempHidden;
  tempHidden.push_back (hiddenSize);
  constructNetwork (inputSize, outputSize, tempHidden);
}

NeuralNetworkVector::NeuralNetworkVector (int inputSize, int outputSize, vector<int> & hiddenSizes)
{
  constructNetwork (inputSize, outputSize, hiddenSizes);
}

NeuralNetworkVector::NeuralNetworkVector (std::istream & stream)
{
  read (stream);
}

void
NeuralNetworkVector::constructNetwork (int inputSize, int outputSize, vector<int> & hiddenSizes)
{
  destroyNetwork ();

  for (int i = 0; i < inputSize; i++)
  {
	inputs.push_back (new NeuronInputVector (inVector, i));
  }
  for (int i = 0; i < outputSize; i++)
  {
	outputs.push_back (new NeuronOutputVector (outVector, i));
  }

  constructHiddenLayers (hiddenSizes);

  this->hiddenSizes = hiddenSizes;
}

void
NeuralNetworkVector::getOutput ()
{
  outVector.resize (outputs.size ());
  for (int i = 0; i < outputs.size (); i++)
  {
	outVector[i] = outputs[i]->getOutput ();
  }
}

void
NeuralNetworkVector::read (std::istream & stream)
{
  cerr << "Warning: NeuralNetworkVector can only read a fully connected network." << endl;

  int inputSize;
  int outputSize;
  stream.read ((char *) & inputSize,  sizeof (inputSize));
  stream.read ((char *) & outputSize, sizeof (outputSize));

  int hiddenLayers;
  stream.read ((char *) & hiddenLayers, sizeof (hiddenLayers));
  vector<int> hiddenSizes (hiddenLayers);
  for (int i = 0; i < hiddenLayers; i++)
  {
	stream.read ((char *) & hiddenSizes[i], sizeof (hiddenSizes[i]));
  }

  constructNetwork (inputSize, outputSize, hiddenSizes);

  // Now read all the weights
  Vector<float> biases;
  Matrix<float> weights;
  if (hiddenLayers < 1)
  {
	biases.read (stream);
	weights.read (stream);
	for (int i = 0; i < outputs.size (); i++)
	{
	  vector<Synapse *> & synapses = outputs[i]->inputs;
	  synapses[0]->weight = biases[i];
	  for (int j = 1; j < synapses.size (); j++)
	  {
		synapses[j]->weight = weights(i,j-1);
	  }
	}
  }
  else
  {
	// Inputs to first hidden layer
	biases.read (stream);
	weights.read (stream);
	for (int i = 0; i < hiddenSizes[0]; i++)
	{
	  vector<Synapse *> & synapses = hidden[i]->inputs;
	  synapses[0]->weight = biases[i];
	  for (int j = 1; j < synapses.size (); j++)
	  {
		synapses[j]->weight = weights(i,j-1);
	  }
	}

	// Hidden layer to hidden layer
	int h = hiddenSizes[0];
	for (int i = 1; i < hiddenLayers; i++)
	{
	  biases.read (stream);
	  weights.read (stream);
	  for (int j = 0; j < hiddenSizes[i]; j++)
	  {
		vector<Synapse *> & synapses = hidden[h++]->inputs;
		synapses[0]->weight = biases[j];
		for (int k = 1; k < synapses.size (); k++)
		{
		  synapses[k]->weight = weights(j,k-1);
		}
	  }
	}

	// Hidden layer to output layer
	biases.read (stream);
	weights.read (stream);
	for (int i = 0; i < outputs.size (); i++)
	{
	  vector<Synapse *> & synapses = outputs[i]->inputs;
	  synapses[0]->weight = biases[i];
	  for (int j = 1; j < synapses.size (); j++)
	  {
		synapses[j]->weight = weights(i,j-1);
	  }
	}
  }
}

void
NeuralNetworkVector::write (std::ostream & stream, bool withName) const
{
  cerr << "Warning: NeuralNetworkVector can only write a fully connected network." << endl;

  NeuralNetworkBackprop::write (stream, withName);

  int inputSize = inputs.size ();
  int outputSize = outputs.size ();
  stream.write ((char *) & inputSize,  sizeof (inputSize));
  stream.write ((char *) & outputSize, sizeof (outputSize));

  int hiddenLayers = hiddenSizes.size ();
  stream.write ((char *) & hiddenLayers, sizeof (hiddenLayers));
  for (int i = 0; i < hiddenLayers; i++)
  {
	stream.write ((char *) & hiddenSizes[i], sizeof (hiddenSizes[i]));
  }

  // Write all the weights
  Vector<float> biases;
  Matrix<float> weights;
  if (hiddenLayers < 1)
  {
	biases.resize (outputs.size ());
	weights.resize (outputs.size (), inputs.size ());
	for (int i = 0; i < outputs.size (); i++)
	{
	  vector<Synapse *> & synapses = outputs[i]->inputs;
	  biases[i] = synapses[0]->weight;
	  for (int j = 1; j < synapses.size (); j++)
	  {
		weights(i,j-1) = synapses[j]->weight;
	  }
	}
	biases.write (stream);
	weights.write (stream);
  }
  else
  {
	// Inputs to first hidden layer
	biases.resize (hiddenSizes[0]);
	weights.resize (hiddenSizes[0], inputs.size ());
	for (int i = 0; i < hiddenSizes[0]; i++)
	{
	  vector<Synapse *> & synapses = hidden[i]->inputs;
	  biases[i] = synapses[0]->weight;
	  for (int j = 1; j < synapses.size (); j++)
	  {
		weights(i,j-1) = synapses[j]->weight;
	  }
	}
	biases.write (stream);
	weights.write (stream);

	// Hidden layer to hidden layer
	int h = hiddenSizes[0];
	for (int i = 1; i < hiddenLayers; i++)
	{
	  biases.resize (hiddenSizes[i]);
	  weights.resize (hiddenSizes[i], hiddenSizes[i-1]);
	  for (int j = 0; j < hiddenSizes[i]; j++)
	  {
		vector<Synapse *> & synapses = hidden[h++]->inputs;
		biases[j] = synapses[0]->weight;
		for (int k = 1; k < synapses.size (); k++)
		{
		  weights(j,k-1) = synapses[k]->weight;
		}
	  }
	  biases.write (stream);
	  weights.write (stream);
	}

	// Hidden layer to output layer
	biases.resize (outputs.size ());
	weights.resize (outputs.size (), hiddenSizes.back ());
	for (int i = 0; i < outputs.size (); i++)
	{
	  vector<Synapse *> & synapses = outputs[i]->inputs;
	  biases[i] = synapses[0]->weight;
	  for (int j = 1; j < synapses.size (); j++)
	  {
		weights(i,j-1) = synapses[j]->weight;
	  }
	}
	biases.write (stream);
	weights.write (stream);
  }
}


// class NeuronInputVector ----------------------------------------------------

NeuronInputVector::NeuronInputVector (Vector<float> & value, int row)
{
  this->value = &value;
  this->row   = row;
}

float
NeuronInputVector::getOutput ()
{
  return activation = (*value)[row];
}


// class NeuronOutputVector ---------------------------------------------------

NeuronOutputVector::NeuronOutputVector (Vector<float> & value, int row)
{
  this->value = &value;
  this->row   = row;
}

float
NeuronOutputVector::getDelta ()
{
  if (isnan (delta))
  {
	float desired = (*value)[row];
	delta = desired - getOutput ();
  }

  return delta;
}
