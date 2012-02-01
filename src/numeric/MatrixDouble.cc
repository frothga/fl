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


#include "fl/Matrix.tcc"
#include "fl/metadata.h"


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
  getString (name, temp);
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
  setString (name, temp);
}
