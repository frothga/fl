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
