/*
Author: Fred Rothganger

Copyright 210 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/match.h"
#include "fl/lapack.h"


using namespace std;
using namespace fl;


// class Match ----------------------------------------------------------------

Match::~Match ()
{
}


// class MatchSet -------------------------------------------------------------

MatchSet::MatchSet ()
{
  model = 0;
}

MatchSet::~MatchSet ()
{
  delete model;
}

void
MatchSet::clear (bool destruct)
{
  if (destruct)
  {
	iterator i = begin ();
	while (i < end ())
	{
	  delete *i++;
	}
  }
  vector<Match *>::clear ();
}

void
MatchSet::set (Registration * model)
{
  delete this->model;
  this->model = model;
}


// class Registration ---------------------------------------------------------

Registration::~Registration ()
{
}


// class RegistrationMethod ---------------------------------------------------

RegistrationMethod::~RegistrationMethod ()
{
}


// class MatchFilter ----------------------------------------------------------

MatchFilter::MatchFilter (RegistrationMethod * method)
: method (method)
{
}

MatchFilter::~MatchFilter ()
{
  delete method;
}


// class MatchFinder ----------------------------------------------------------

MatchFinder::~MatchFinder ()
{
}


// class Homography -----------------------------------------------------------

double
Homography::test (const Match & match) const
{
  Vector<double> b = H * match[1]->homogeneous ();
  b /= b[2];
  return (*match[0] - b).norm (2);
}


// class HomographyMethod -----------------------------------------------------

HomographyMethod::HomographyMethod (int dof)
: dof (dof)
{
}

Registration *
HomographyMethod::construct (const MatchSet & matches) const
{
  Homography * result = new Homography;
  Matrix<double> & H = result->H;
  H.resize (3, 3);
  H.identity ();
  const int count = matches.size ();
  
  switch (dof)
  {
	case 2: // Pure translation
	case 3: // Translation and rotation
	{
	  Point sum0;
	  Point sum1;
	  sum0.clear ();
	  sum1.clear ();
	  for (int i = 0; i < count; i++)
	  {
		Match & m = *matches[i];
		sum0 += *m[0];
		sum1 += *m[1];
	  }
	  sum0 /= count;
	  sum1 /= count;
	  H.region (0,2) = sum0 - sum1;
	  if (dof == 2) return result;

	  double angle = 0;
	  for (int i = 0; i < count; i++)
	  {
		Match & m = *matches[i];
		double a = sum0.angle (*m[0]) - sum1.angle (*m[1]);
		if      (a >  M_PI) a -= TWOPI;
		else if (a < -M_PI) a += TWOPI;
		angle += a;
	  }
	  angle /= count;
	  H(0,0) = H(1,1) = cos (angle);
	  H(1,0) = sin (angle);
	  H(0,1) = -H(1,0);
	  return result;
	}
	case 4:
	{
	  Matrix<double> A (count * 2, 4);
	  Vector<double> b (count * 2);
	  A.clear ();
	  int r = 0;  // row
	  for (int i = 0; i < count; i++)
	  {
		Match & m = *matches[i];
		Point & p0 = *m[0];
		Point & p1 = *m[1];
		A(r,0) = p1.x;
		A(r,1) = 1;
		b[r]   = p0.x;
		r++;
		A(r,2) = p1.y;
		A(r,3) = 1;
		b[r]   = p0.y;
		r++;
	  }
	  Matrix<double> x;
	  gelss (A, x, b);
	  H(0,0) = x[0];
	  H(0,2) = x[1];
	  H(1,1) = x[2];
	  H(1,2) = x[3];
	  return result;
	}
	case 6:
	{
	  Matrix<double> A (count * 2, 6);
	  Vector<double> b (count * 2);
	  A.clear ();
	  int r = 0;  // row
	  for (int i = 0; i < count; i++)
	  {
		Match & m = *matches[i];
		Point & p0 = *m[0];
		Point & p1 = *m[1];
		A(r,0) = p1.x;
		A(r,1) = p1.y;
		A(r,2) = 1;
		b[r]   = p0.x;
		r++;
		A(r,3) = p1.x;
		A(r,4) = p1.y;
		A(r,5) = 1;
		b[r]   = p0.y;
		r++;
	  }
	  Matrix<double> x;
	  gelss (A, x, b);
	  H.region (0, 0) = ~x.reshape (3, 2);
	  return result;
	}
	case 8:
	{
	  Matrix<double> A (count * 2, 8);
	  Vector<double> b (count * 2);
	  A.clear ();
	  int r = 0;  // row
	  for (int i = 0; i < count; i++)
	  {
		Match & m = *matches[i];
		Point & p0 = *m[0];
		Point & p1 = *m[1];
		A(r,0) =  p1.x;
		A(r,1) =  p1.y;
		A(r,2) =  1;
		A(r,6) = -p0.x * p1.x;
		A(r,7) = -p0.x * p1.y;
		b[r]   =  p0.x;
		r++;
		A(r,3) =  p1.x;
		A(r,4) =  p1.y;
		A(r,5) =  1;
		A(r,6) = -p0.y * p1.x;
		A(r,7) = -p0.y * p1.y;
		b[r]   =  p0.y;
		r++;
	  }
	  Matrix<double> x;
	  gelss (A, x, b);
	  H.region (0,0) = ~x.reshape (3, 2);
	  H(2,0) = x[6];
	  H(2,1) = x[7];
	  return result;
	}
	default:
	{
	  delete result;
	  throw "Unsupported DOF";
	}
  }
}

int
HomographyMethod::minMatches () const
{
  // Depending on the type of the points in the given matches, there may
  // be more information available, and we can use fewer matches.  For example,
  // a single match between PointAffines is enough to establish a 6-dof
  // homography.  On the other hand, estimates of parameters other than
  // position tend to be less reliable.
  return (int) ceil (dof / 2.0);
}


// class Ransac ---------------------------------------------------------------

Ransac::Ransac (RegistrationMethod * method)
: MatchFilter (method)
{
  k = -4;
  w = 0.1;
  p = 0.99;
  t = 1;
  d = method->minMatches ();
}

void
Ransac::run (const MatchSet & source, MatchSet & result) const
{
  // Determine number of iterations
  int n = method->minMatches ();
  int K = k;
  if (k < 0)
  {
	double wn = pow (w, n);
	double sdk = sqrt (1 - wn);  // standard deviation of k
	K = (int) ceil ((1 - k * sdk) / wn);
  }

  int count = source.size ();
  MatchSet work = source;
  Registration * registration = 0;
  int biggestConsensus = 0;
  result.clear ();
  for (int i = 0; i < K; i++)
  {
	// Permute working set to generate random sample
	for (int r = 0; r < n; r++)
	{
	  int index = rand () % count;
	  swap (work[r], work[index]);
	}
	MatchSet sample;
	MatchSet::iterator wb = work.begin ();
	sample.insert (sample.begin (), wb, wb + n);

	// Compute model and get consensus set
	delete registration;
	registration = method->construct (sample);
	if (registration->error > t) continue;
	for (int j = n; j < count; j++)
	{
	  Match * m = work[j];
	  if (registration->test (*m) <= t) sample.push_back (m);
	}

	// Evaluate consensus set
	int consensus = sample.size () - n;
	if (consensus < d) continue;
	if (consensus > biggestConsensus)
	{
	  biggestConsensus = consensus;
	  result = sample;
	  result.set (registration);
	  registration = 0;
	}
  }
  delete registration;
}


// class FixedPoint -----------------------------------------------------------

FixedPoint::FixedPoint (RegistrationMethod * method)
: MatchFilter (method)
{
  maxIterations = 20;
  t = 1;
}

void
FixedPoint::run (const MatchSet & source, MatchSet & result) const
{
  int n = method->minMatches ();
  int count = source.size ();
  int i = 0;
  int oldSize = 0;
  int newSize = result.size ();
  while (i < maxIterations  &&  newSize != oldSize  &&  newSize >= n)
  {
	oldSize = newSize;
	Registration * model = method->construct (result);
	result.set (model);
	result.clear ();
	for (int i = 0; i < count; i++)
	{
	  Match * m = source[i];
	  if (model->test (*m) < t) result.push_back (m);
	}
	newSize = result.size ();
  }
}


// class NearestDescriptors ---------------------------------------------------

NearestDescriptors::NearestDescriptors (PointSet & reference)
{
  set (reference);
}

NearestDescriptors::~NearestDescriptors ()
{
  clear ();
}

void
NearestDescriptors::clear ()
{
  for (int i = 0; i < data.size (); i++) delete data[i];
  data.clear ();
}

void
NearestDescriptors::set (PointSet & reference)
{
  data.clear ();
  data.reserve (reference.size ());
  PointSet::iterator i = reference.begin ();
  for (; i != reference.end (); i++)
  {
	Point * p = *i;
	Vector<float> * descriptor = p->descriptor ();
	if (! descriptor) continue;
	data.push_back (new Neighbor::Entry (descriptor, p));
  }
  tree.clear ();
  tree.bucketSize = 2;
  tree.k = 2;
  tree.set (data);
}

void
NearestDescriptors::run (PointSet & query, MatchSet & result) const
{
  PointSet::iterator it;
  for (it = query.begin (); it != query.end (); it++)
  {
	Point * p = *it;
	Vector<float> * descriptor = p->descriptor ();
	if (! descriptor) continue;

	vector<MatrixAbstract<float> *> answer;
	tree.find (*descriptor, answer);
	if (answer.size () < 2) continue;
	Neighbor::Entry * a0 = (Neighbor::Entry *) answer[0];
	Neighbor::Entry * a1 = (Neighbor::Entry *) answer[1];
	double d0 = (*a0 - *descriptor).norm (2);
	if (d0 > threshold) continue;
	double d1 = (*a1 - *descriptor).norm (2);
	if (d0 / d1 > 0.8) continue;

	Match * m = new Match;
	m->resize (2);
	(*m)[0] = p;
	(*m)[1] = (Point *) a0->item;
	result.push_back (m);
  }
}
