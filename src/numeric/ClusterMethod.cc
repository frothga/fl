/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


08/2005 Fred Rothganger -- Compilability fix for GCC 3.4.4
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/cluster.h"
#include "fl/factory.h"


using namespace fl;
using namespace std;


// class ClusterMethod --------------------------------------------------------

template class Factory<ClusterMethod>;
template <> Factory<ClusterMethod>::productMap Factory<ClusterMethod>::products;

void
ClusterMethod::read (istream & stream)
{
}

void
ClusterMethod::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
