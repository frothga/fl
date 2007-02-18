/*
Author: Fred Rothganger
Created 01/12/2005 to support compilation under MSVC.


Revisions 1.1 thru 1.3 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/08 18:19:24  Fred
Add revision history.

Revision 1.2  2005/01/22 20:38:02  Fred
MSVC compilability fix: Use template<> to introduce template specializations. 
Move Matrix<bool> into a different file to get rid of multiple definitions.

Revision 1.1  2005/01/12 05:44:18  rothgang
Add template specializations for bool.  Need to move MatrixSparseBool code into
separate file, because it creates duplicate instantiations.
-------------------------------------------------------------------------------
*/


#ifndef fl_matrix_bool_tcc
#define fl_matrix_bool_tcc


#include "fl/matrix.h"


namespace fl
{
  template<>
  bool
  MatrixAbstract<bool>::frob (float n) const
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
  Matrix<bool>::frob (float n) const
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
