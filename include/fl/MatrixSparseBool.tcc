/*
Author: Fred Rothganger
Created 01/22/2005 to support compilation under MSVC.


Revision 1.1 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.2  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.1  2005/08/28 23:40:08  Fred
Specialized template to avoid compilation issues with bool.
-------------------------------------------------------------------------------
*/


#ifndef fl_matrix_sparse_bool_tcc
#define fl_matrix_sparse_bool_tcc


#include "fl/MatrixSparse.tcc"


namespace fl
{
  template<>
  bool
  MatrixSparse<bool>::frob (float n) const
  {
	int w = data->size ();

	for (int c = 0; c < w; c++)
	{
	  std::map<int,bool> & C = (*data)[c];
	  std::map<int,bool>::iterator i = C.begin ();
	  while (i != C.end ())
	  {
		if (i->second) return true;
		i++;
	  }
	}

	return false;
  }
}


#endif
