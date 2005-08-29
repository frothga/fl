/*
Author: Fred Rothganger
Created 01/22/2005 to support compilation under MSVC.
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
