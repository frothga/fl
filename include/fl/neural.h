#ifndef fl_neural_h
#define fl_neural_h

#include "fl/matrix.h"

#include <vector>
#include <map>
#include <iostream>


namespace fl
{
  // Basic neural network elements --------------------------------------------

  class Synapse;

  /**
	 A node in a neural network.  Integrates information from other Neurons
	 via Synapse's and transmits results out over other Synapse's.
   **/
  class Neuron
  {
  public:
	virtual ~Neuron ();

	std::vector<Synapse *> outputs;
	std::vector<Synapse *> inputs;
  };

  /**
	 Computes the neuron's response by summing inputs and passing thru
	 a squashing function.  Performs back-propogation learning.  Works in
	 a rigid one-shot fashion, so must be reset and reloaded to handle
	 mutliple data or points in time.

	 To construct an NN that works on real world data and has useful output,
	 you should create two subclasses of this class:
	 * "NeuronInput" -- Computes an appropriate output level based on current
	   input datum.  Only need to override getOutput().  No need for
	   afferent Synapses.
	 * "NeuronOutput" -- Computes an appropriate error signal based on the
	   expected value from a training datum.  Only need to override getDelta().

	 Note that hidden layers in an NN would be constructed directly out of
	 instances of this class or some specialized subclass.  An example of
	 specialization is the NeuronDelay below.
   **/
  class NeuronBackprop : public Neuron
  {
  public:
	NeuronBackprop ();

	virtual void  startCycle ();     ///< Clear latches
	float         getActivation ();  ///< Add up net activation level from all synapses
	virtual float getOutput ();      ///< Compute output of squashing function.  Draws on net activation.
	virtual float getDelta ();       ///< Collect error from subsequent Neurons via back-propogation.
	float         getError ();       ///< Compute the error signal based on delta.
	virtual void  learn ();          ///< Request all Synapses feeding this Neuron to adjust their weights according to the error signal.

	float activation;  ///< net activation level (before squashing function) for current cycle.  Before it is calculated, its value is nan.
	float delta;  ///< error signal for current cycle.  Before it is calculated, its value is nan.
  };

  /**
	 Uses the net activation from the previous cycle to determine output,
	 rather than net activation from current cycle.
   **/
  class NeuronDelay : public NeuronBackprop
  {
  public:
	NeuronDelay ();

	virtual void  startCycle ();
	virtual float getOutput ();
	virtual float getDelta ();

	float lastActivation;
  };

  /**
	 An input Neuron that reads an entry in a vector.  Allows the programmer
	 to devise an NN that reads feature vectors directly.
   **/
  class NeuronInputVector : public NeuronBackprop
  {
  public:
	NeuronInputVector (Vector<float> & value, int row);

	virtual float getOutput ();  ///< Grabs value[row].

	Vector<float> * value;  ///< Pointer to a shared Vector<float> that is updated by client code.
	int row;  ///< The position in the vector that this specific Neuron presents to the NN.
  };

  /**
	 An output Neuron that uses an entry in a vector as ground truth during
	 training.  Doesn't actually write to the vector.
   **/
  class NeuronOutputVector : public NeuronBackprop
  {
  public:
	NeuronOutputVector (Vector<float> & value, int row);

	virtual float getDelta ();  ///< Compares output to value[row].

	Vector<float> * value;  ///< Pointer to a shared Vector<float> that is updated by client code.
	int row;  ///< The position in the vector that this specific Neuron presents to the NN.
  };

  /**
	 One-way connection between two Neurons.  Holds the synaptic weight.
   **/
  class Synapse
  {
  public:
	Synapse ();
	Synapse (Neuron * from, Neuron * to);
	Synapse (Neuron * from, Neuron * to, float weight);
	virtual ~Synapse ();
	void initialize (Neuron * from, Neuron * to, float weight);

	float    weight;
	Neuron * from;
	Neuron * to;
  };

  /**
	 Conveys one-shot computation forward, and adjusts weight during
	 back-prop learning.
   **/
  class SynapseBackprop : public Synapse
  {
  public:
	SynapseBackprop () {};
	SynapseBackprop (NeuronBackprop * from, NeuronBackprop * to) : Synapse (from, to) {};
	SynapseBackprop (NeuronBackprop * from, NeuronBackprop * to, float weight) : Synapse (from, to, weight) {};

	float         getError ();
	virtual float getOutput ();
	virtual void  learn ();
	bool          isActivationValid ();

	static float eta;  ///< Learning rate.
	static float largestChange;
  };

  /**
	 Applies the bias level for a Neuron.  This is an elegant alternative
	 to fixing the bias as an attribute of the Neuron itself, and it allows
	 back-prop to learn the bias value.
   **/
  class SynapseBias : public SynapseBackprop
  {
  public:
	SynapseBias (NeuronBackprop * to) : SynapseBackprop (NULL, to) {};

	float getOutput ();
	void  learn ();
  };


  // Neural network engine ----------------------------------------------------

  class NeuralNetwork
  {
  public:
	virtual ~NeuralNetwork ();  ///< Establishes that destructor is virtual, but doesn't do anything.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false) const;
  };

  /**
	 The most common type of NN: does feedforward function evaluation and
	 back-prop learning.  Use the constructor in a derived class to build
	 the NN, and override appropriate functions to supply data during
	 training or runtime phases.
   **/
  class NeuralNetworkBackprop : public NeuralNetwork
  {
  public:
	virtual ~NeuralNetworkBackprop ();
	void destroyNetwork ();

	void constructHiddenLayer (int n);  ///< Assuming only inputs and outputs are already set up, add one layer of hidden units and fully connect everything.
	void constructHiddenLayers (std::vector<int> & sizes);  ///< Given that inputs and outputs are already set up and no hidden elements exist yet, construct a fully connected network with an arbitrary number of layers.

	void train (float tolerance = 1e-4);  ///< Repeatedly loop thru a set of input data, running the backprop algorithm, until the mean squared error of the output neurons stabilizes.
	virtual void startData () = 0;  ///< Called by train() to signal the start of a cycle thru the data.  This function should reset any indices in preparation for calls to nextDatum().
	virtual bool nextDatum () = 0;  ///< Called by train() to request that the next training point be set up on the inputs and outputs of the NN.  Return true if a new datum was set up.  Return false if a datum is not available, indicating the end of the data set.  Similar semantics to end of file flag.
	virtual bool correct ();  ///< Determines if the outputs are correct (by whatever contorted method).  Called by train() to support the construction of "happy graphs", but not necessary to implement.
	virtual void happyGraph (int iteration, float accuracy);  ///< Called by train() to support user level construction of learning curve graph.  "iteration" counts the number of cycles thru the data so far, and "accuracy" is a number between 0 and 1, where 1 is perfect.

	void reset ();  ///< Clear latches in preparation for one-shot NN computation.  Should be done each time you change data on the inputs and want to get a fresh reading from the outputs.

	std::vector<NeuronBackprop *> inputs;
	std::vector<NeuronBackprop *> outputs;
	std::vector<NeuronBackprop *> hidden;  ///< Can include more than one hidden layer.
  };

  /**
	 A backprop NN that binds its input and outputs to Vector<float>'s.
   **/
  class NeuralNetworkVector : public NeuralNetworkBackprop
  {
  public:
	NeuralNetworkVector (int inputSize, int outputSize, int hiddenSize);
	NeuralNetworkVector (int inputSize, int outputSize, std::vector<int> & hiddenSizes);
	NeuralNetworkVector (std::istream & stream);
	void constructNetwork (int inputSize, int outputSize, std::vector<int> & hiddenSizes);

	void getOutput ();  ///< Transfers values from output layer to outVector.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false) const;

	Vector<float> inVector;  ///< Input neurons are bound directly to the contents of this vector.
	Vector<float> outVector;  ///< The ground truth for the output neurons is bound directly to this vector.  Alternately, if you call getOutput(), the output values overwrite the data in this vector.
	std::vector<int> hiddenSizes;  ///< Record of the structure of the hidden layer(s).
  };
}


#endif
