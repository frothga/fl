/*
Author: Fred Rothganger
Created 01/12/2005 to support compilation under MSVC.


Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_matrix_bool_tcc
#define fl_matrix_bool_tcc


#include "fl/matrix.h"


namespace fl
{
  template<>
  bool
  MatrixAbstract<bool>::norm (float n) const
  {
	int h = rows ();
	int w = columns ();

	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		if ((*this)(r,c)) return true;
	  }
	}
	return false;
  }

  template<>
  bool
  MatrixStrided<bool>::norm (float n) const
  {
	bool * i = (bool *) data;
	bool * end = i + rows_ * columns_;

	while (i < end)
	{
	  if (*i++) return true;
	}
	return false;
  }
}


#endif
