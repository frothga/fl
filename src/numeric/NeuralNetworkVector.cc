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
  if (hiddenSizes.size () < 1)
  {
	Vector<float> biases (stream);
	Matrix<float> weights (stream);
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
	Vector<float> biases;
	Matrix<float> weights;

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
	for (int i = 1; i < hiddenSizes.size (); i++)
	{
	  biases.read (stream);
	  weights.read (stream);
	  for (int j = 0; j < hiddenSizes[i]; j++)
	  {
		vector<Synapse *> & synapses = hidden[h+j]->inputs;
		synapses[0]->weight = biases[j];
		for (int k = 1; k < synapses.size (); k++)
		{
		  synapses[k]->weight = weights(j,k-1);
		}
	  }
	  h += hiddenSizes[i];
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
}

void
NeuralNetworkVector::write (std::ostream & stream, bool withName) const
{
  cerr << "Warning: NeuralNetworkVector can only write a fully connected network." << endl;

  NeuralNetworkBackprop::write (stream, withName);

  int inputSize = inputs.size ();
  int outputSize = outputs.size ();
  int hiddenSize = hidden.size ();
  stream.write ((char *) & inputSize,  sizeof (inputSize));
  stream.write ((char *) & outputSize, sizeof (outputSize));
  stream.write ((char *) & hiddenSize, sizeof (hiddenSize));

  // Write all the weights
  Matrix<float> inputHiddenWeights (hiddenSize, inputSize);
  Matrix<float> hiddenOutputWeights (outputSize, hiddenSize);

  for (int i = 0; i < inputSize; i++)
  {
	vector<Synapse *> & synapses = inputs[i]->outputs;
	for (int j = 0; j < hiddenSize; j++)
	{
	  inputHiddenWeights(j,i) = synapses[j]->weight;
	}
  }

  for (int i = 0; i < hiddenSize; i++)
  {
	vector<Synapse *> & synapses = hidden[i]->outputs;
	for (int j = 0; j < outputSize; j++)
	{
	  hiddenOutputWeights(j,i) = synapses[j]->weight;
	}
  }

  inputHiddenWeights.write (stream);
  hiddenOutputWeights.write (stream);
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
