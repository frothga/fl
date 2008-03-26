/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.2  2004/04/19 16:34:04  rothgang
Add MDA, and expand DimensionalityReduction interface to include class
assignments needed by MDA.  Rename PCA.

Revision 1.1  2004/04/06 20:10:49  rothgang
Create class of algorithms for reducing dimensionality.
-------------------------------------------------------------------------------
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
	virtual void write (std::ostream & stream) const;
  };

  class PCA : public DimensionalityReduction
  {
  public:
	PCA (int targetDimension);
	PCA (std::istream & stream);

	virtual void analyze (const std::vector< Vector<float> > & data);
	virtual Vector<float> reduce (const Vector<float> & datum);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

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
	virtual void write (std::ostream & stream) const;

	Matrix<float> W;  ///< Basis matrix for reduced space.
  };
}


#endif
