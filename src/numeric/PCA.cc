/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


09/2005 Fred Rothganger -- Change lapacks.h to lapack.h
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
  syev (covariance, eigenvalues, eigenvectors);

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
