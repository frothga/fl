#ifndef fl_matrix_bool_tcc
#define fl_matrix_bool_tcc


#include "fl/matrix.h"


namespace fl
{
  bool
  MatrixAbstract<bool>::frob (bool n) const
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

  bool
  Matrix<bool>::frob (bool n) const
  {
	bool * i = (bool *) data;
	bool * end = i + rows_ * columns_;

	while (i < end)
	{
	  if (*i++) return true;
	}
	return false;
  }

  bool
  MatrixSparse<bool>::frob (bool n) const
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
