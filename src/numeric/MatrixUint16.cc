/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


/// A small cheat to make the basic norm() template work.
#include <stdint.h>
namespace std
{
  inline uint16_t
  max (const int & a, const uint16_t & b)
  {
	return std::max (a, (int) b);
  }
}

#include "fl/Matrix.tcc"


using namespace fl;


template class MatrixAbstract<uint16_t>;
template class MatrixStrided<uint16_t>;
template class Matrix<uint16_t>;
template class MatrixTranspose<uint16_t>;
template class MatrixRegion<uint16_t>;

namespace fl
{
  template SHARED std::ostream & operator << (std::ostream & stream, const MatrixAbstract<uint16_t> & A);
  template SHARED std::istream & operator >> (std::istream & stream, MatrixAbstract<uint16_t> & A);
  template SHARED MatrixAbstract<uint16_t> & operator << (MatrixAbstract<uint16_t> & A, const std::string & source);
}
