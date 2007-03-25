/*
Author: Fred Rothganger
Created 01/30/2006 to provide a general interface for measuring distances
in R^n, and to help separate numeric and image libraries.


Revisions 1.1 thru 1.3 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.3  2007/03/25 14:06:45  Fred
Fix template instantiation for Factory.

Revision 1.2  2007/03/23 10:57:27  Fred
Use CVS Log to generate revision history.

Revision 1.1  2006/02/05 23:40:13  Fred
Add new class to represent distances in R^n, and to help remove the dependency
of this numeric library on the image library.
-------------------------------------------------------------------------------
*/


#include "fl/metric.h"
#include "fl/factory.h"


using namespace fl;
using namespace std;


// class Metric ---------------------------------------------------------------

template class Factory<Metric>;

Metric::~Metric ()
{
}

void
Metric::read (istream & stream)
{
}

void
Metric::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
