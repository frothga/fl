/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


09/2005 Fred Rothganger -- Change lapackd.h to lapack.h
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
*/


#include <fl/Matrix.tcc>
#include <fl/lapack.h>


using namespace std;
using namespace fl;


int
main (int argc, char * argv[])
{
  Matrix<double> A;

  cin >> A;
  cout << !A << endl;

  return 0;
}
