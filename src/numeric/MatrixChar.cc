#include "fl/Matrix.tcc"


using namespace fl;


std::ostream &
fl::operator << (std::ostream & stream, const MatrixAbstract<char> & A)
{
  for (int r = 0; r < A.rows (); r++)
  {
	if (r > 0)
	{
	  if (A.columns () > 1)
	  {
		stream << std::endl;
	  }
	  else  // This is really a vector, so don't break lines.
	  {
		stream << " ";
	  }
	}
	std::string line;
	for (int c = 0; c < A.columns (); c++)
	{
	  std::ostringstream formatted;
	  formatted.precision (A.displayPrecision);
	  formatted << (int) A (r, c);
	  if (c > 0)
	  {
		line += ' ';
	  }
	  while (line.size () < c * A.displayWidth)
	  {
		line += ' ';
	  }
	  line += formatted.str ();
	}
	stream << line;
  }
  return stream;
}


template class MatrixAbstract<char>;
template class Matrix<char>;
template class MatrixTranspose<char>;
template class MatrixRegion<char>;
