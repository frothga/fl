/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"
#include "fl/serialize.h"


using namespace std;
using namespace fl;


// class Descriptor -----------------------------------------------------------

template class Factory<Descriptor>;

Descriptor::Descriptor ()
{
  monochrome = true;
  dimension = 0;
  supportRadial = 0;
}

Descriptor::~Descriptor ()
{
}

Vector<float>
Descriptor::value (const Image & image)
{
  throw "alpha selected regions not implemented";
}

Comparison *
Descriptor::comparison ()
{
  return new NormalizedCorrelation;
}

void
Descriptor::read (istream & stream)
{
}

void
Descriptor::write (ostream & stream) const
{
}
