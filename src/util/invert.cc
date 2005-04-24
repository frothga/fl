/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include <fl/Matrix.tcc>
#include <fl/lapackd.h>


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
