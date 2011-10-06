/*
Author: Fred Rothganger
Created 01/30/2006 to provide a general interface for measuring distances
in R^n, and to help separate numeric and image libraries.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/metric.h"
#include "fl/serialize.h"


using namespace fl;
using namespace std;


// class Metric ---------------------------------------------------------------

uint32_t Metric::serializeVersion = 0;

Metric::~Metric ()
{
}

void
Metric::serialize (Archive & archive, uint32_t version)
{
}
