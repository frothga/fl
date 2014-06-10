/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#define flNumeric_MS_EVIL
#include "fl/MatrixDouble.tcc"
#include "fl/metadata.h"

// Must redefine SHARED, because metadata.h includes matrix.h and then sets
// SHARED for base rather than numeric.  Changing include order doesn't help,
// due to inclusion guard.
#undef SHARED
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


using namespace fl;
using namespace std;


template class MatrixAbstract<double>;
template class MatrixStrided<double>;
template class Matrix<double>;
template class MatrixTranspose<double>;
template class MatrixRegion<double>;

namespace fl
{
  template SHARED std::ostream & operator << (std::ostream & stream, const MatrixAbstract<double> & A);
  template SHARED std::istream & operator >> (std::istream & stream, MatrixAbstract<double> & A);
  template SHARED MatrixAbstract<double> & operator << (MatrixAbstract<double> & A, const std::string & source);
}

void
Metadata::get (const string & name, Matrix<double> & value)
{
  string temp;
  get (name, temp);
  if (temp.size ())
  {
	if (temp.find_first_of ('[') != string::npos)
	{
	  value = Matrix<double> (temp);
	}
	else
	{
	  value.resize (1, 1);
	  value(0,0) = atof (temp.c_str ());
	}
  }
}

void
Metadata::set (const string & name, const Matrix<double> & value)
{
  string temp;
  value.toString (temp);
  set (name, temp);
}
