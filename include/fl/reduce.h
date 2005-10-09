/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
*/


#ifndef fl_reduce_h
#define fl_reduce_h


#include <fl/matrix.h>

#include <iostream>


namespace fl
{
  /**
	 Dimensionality reduction methods.
  **/
  class DimensionalityReduction
  {
  public:
	// A derived class must override at least one analyze() method.  Otherwise,
	// the default implementations will create an infinite loop.
	virtual void analyze (const std::vector< Vector<float> > & data);
	virtual void analyze (const std::vector< Vector<float> > & data, const std::vector<int> & classAssignments);
	virtual Vector<float> reduce (const Vector<float> & datum) = 0;

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);
  };

  class PCA : public DimensionalityReduction
  {
  public:
	PCA (int targetDimension);
	PCA (std::istream & stream);

	virtual void analyze (const std::vector< Vector<float> > & data);
	virtual Vector<float> reduce (const Vector<float> & datum);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);

	int targetDimension;
	Matrix<float> W;  ///< Basis matrix for reduced space.
  };

  class MDA : public DimensionalityReduction
  {
  public:
	MDA () {}
	MDA (std::istream & stream);

	virtual void analyze (const std::vector< Vector<float> > & data, const std::vector<int> & classAssignments);
	virtual Vector<float> reduce (const Vector<float> & datum);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);

	Matrix<float> W;  ///< Basis matrix for reduced space.
  };
}


#endif
