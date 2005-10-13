/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
*/


#include "fl/neural.h"

#include <math.h>


using namespace std;
using namespace fl;


// class NeuralNetwork --------------------------------------------------------

NeuralNetwork::~NeuralNetwork ()
{
}

void
NeuralNetwork::read (std::istream & stream)
{
}

void
NeuralNetwork::write (std::ostream & stream, bool withName) const
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}


// class Neuron --------------------------------------------------------------

Neuron::~Neuron ()
{
  vector<Synapse *>::iterator i;
  for (i = inputs.begin (); i < inputs.end (); i++)
  {
	(*i)->to = NULL;
	delete *i;
  }
  for (i = outputs.begin (); i < outputs.end (); i++)
  {
	(*i)->from = NULL;
	delete *i;
  }
}


// class Synapse -------------------------------------------------------------

Synapse::Synapse ()
{
  from = NULL;
  to = NULL;
}

Synapse::Synapse (Neuron * from, Neuron * to)
{
  initialize
  (
    from,
	to,
	2.0 * (rand () - RAND_MAX / 2) / RAND_MAX
  );
}

Synapse::Synapse (Neuron * from, Neuron * to, float weight)
{
  initialize
  (
    from,
	to,
	weight
  );
}

void
Synapse::initialize (Neuron * from, Neuron * to, float weight)
{
  this->weight = weight;
  this->from = from;
  this->to = to;

  if (from)
  {
	from->outputs.push_back (this);
  }
  if (to)
  {
	to->inputs.push_back (this);
  }
}

Synapse::~Synapse ()
{
  vector<Synapse *>::iterator i;
  if (from)
  {
	for (i = from->outputs.begin (); i < from->outputs.end (); i++)
	{
	  if (*i == this)
	  {
		from->outputs.erase (i);
		break;
	  }
	}
  }
  if (to)
  {
	for (i = to->inputs.begin (); i < to->inputs.end (); i++)
	{
	  if (*i == this)
	  {
		to->inputs.erase (i);
		break;
	  }
	}
  }
}
