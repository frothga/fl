#ifndef fl_reduce_h
#define fl_reduce_h


/**
   Dimensionality reduction methods.
**/

#include <fl/matrix.h>

#include <iostream>


namespace fl
{
  class DimensionalityReduction
  {
  public:
	virtual void analyze (const std::vector< Vector<float> > & data) = 0;
	virtual Vector<float> reduce (const Vector<float> & datum) = 0;

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);
  };

  class DimensionalityReductionPCA : public DimensionalityReduction
  {
  public:
	DimensionalityReductionPCA (int targetDimension);
	DimensionalityReductionPCA (std::istream & stream);

	virtual void analyze (const std::vector< Vector<float> > & data);
	virtual Vector<float> reduce (const Vector<float> & datum);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);

	int targetDimension;
	Matrix<float> W;  ///< Basis matrix for reduced space.
  };
}


#endif
