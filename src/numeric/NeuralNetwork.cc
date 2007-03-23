/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 10:57:28  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.2  2004/04/06 21:17:40  rothgang
Destroy outgoing links as well as incoming links in Neuron.

Revision 1.1  2004/01/06 18:09:15  rothgang
Add neural network framework.
-------------------------------------------------------------------------------
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
