/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/cluster.h"
#include "fl/factory.h"


using namespace fl;
using namespace std;


// class ClusterMethod --------------------------------------------------------

Factory<ClusterMethod>::productMap Factory<ClusterMethod>::products;

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
