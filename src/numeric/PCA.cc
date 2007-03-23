/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.5 Copyright 2005 Sandia Corporation.
Revisions 1.7 and 1.8 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 10:57:30  Fred
Use CVS Log to generate revision history.

Revision 1.7  2006/02/16 04:46:45  Fred
Use destroy option in syev().

Revision 1.6  2005/10/13 04:14:26  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/13 03:43:54  Fred
Add Sandia distribution terms.

Revision 1.4  2005/09/12 03:48:25  Fred
Change lapacks.h to lapack.h

Add Sandia copyright notice.  Must add license info before release.

Revision 1.3  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.2  2004/04/19 17:25:42  rothgang
Change name to PCA.  Fix bug of not clearing mean and covariance matrix before
adding.

Revision 1.1  2004/04/06 21:07:04  rothgang
Add class of algorithms for reducing dimensionality.  First instance is PCA.
-------------------------------------------------------------------------------
*/


#include "fl/reduce.h"
#include "fl/lapack.h"


using namespace std;
using namespace fl;


// class PCA ------------------------------------------------------------------

PCA::PCA (int targetDimension)
{
  this->targetDimension = targetDimension;
}

PCA::PCA (istream & stream)
{
  read (stream);
}

void
PCA::analyze (const vector< Vector<float> > & data)
{
  cerr << "Warning: PCA has not been properly tested yet." << endl;

  int sourceDimension = data[0].rows ();
  int d = min (sourceDimension, targetDimension);

  Vector<float> mean (sourceDimension);
  mean.clear ();
  for (int i = 0; i < data.size (); i++)
  {
	mean += data[i];
  }
  mean /= data.size ();

  Matrix<float> covariance (sourceDimension, sourceDimension);
  covariance.clear ();
  for (int i = 0; i < data.size (); i++)
  {
	Vector<float> delta = data[i] - mean;
	covariance += delta * ~delta;
  }
  covariance /= data.size ();

  Vector<float> eigenvalues;
  Matrix<float> eigenvectors;
  syev (covariance, eigenvalues, eigenvectors, true);

  map<float, int> sorted;
  for (int i = 0; i < eigenvalues.rows (); i++)
  {
	sorted.insert (make_pair (fabsf (eigenvalues[i]), i));
  }

  W.resize (d, sourceDimension);
  map<float, int>::reverse_iterator it = sorted.rbegin ();
  for (int i = 0; i < d  &&  it != sorted.rend (); it++, i++)
  {
	W.row (i) = ~ eigenvectors.column (it->second);
  }
}

Vector<float>
PCA::reduce (const Vector<float> & datum)
{
  return W * datum;
}

void
PCA::read (istream & stream)
{
  stream.read ((char *) & targetDimension, sizeof (targetDimension));
  W.read (stream);
}

void
PCA::write (ostream & stream, bool withName)
{
  DimensionalityReduction::write (stream, withName);

  stream.write ((char *) & targetDimension, sizeof (targetDimension));
  W.write (stream, false);
}
