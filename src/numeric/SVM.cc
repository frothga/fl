/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/cluster.h"
#include "fl/lapack.h"
#include "fl/search.h"

#include <set>


using namespace std;
using namespace fl;


class RBF : public Metric
{
public:
  RBF (float gamma = 1.0f) : gamma (gamma) {}
  virtual float value (const Vector<float> & value1, const Vector<float> & value2) const
  {
	return exp (-gamma * (value1 - value2).sumSquares ());
  }
  void serialize (Archive & archive, uint32_t version)
  {
	archive & *((Metric *) this);
	archive & gamma;
  }
  float gamma;
};


// class SVM ---------------------------------------------------------------

uint32_t SVM::serializeVersion = 0;

SVM::SVM ()
{
  metric = 0;
  epsilon = 1e-3;
}

SVM::~SVM ()
{
  clear ();
  if (metric) delete metric;
}

void
SVM::clear ()
{
  for (int i = 0; i < clusters .size (); i++) delete clusters[i];
  for (int i = 0; i < decisions.size (); i++) delete decisions[i];
  clusters.clear ();
  decisions.clear ();
}

void
SVM::run (const vector<Vector<float> > & data, const vector<int> & classes)
{
  stop = false;
  clear ();

  const int count = min (data.size (), classes.size ());
  const int rows = data[0].rows ();
  if (! metric) metric = new RBF (1.0f / rows);

  // Group training data into classes
  map<int, Cluster *> sorted;
  for (int i = 0; i < count; i++)
  {
	map<int, Cluster *>::iterator it = sorted.find (classes[i]);
	if (it == sorted.end ()) it = sorted.insert (make_pair (classes[i], new Cluster)).first;
	it->second->support.push_back (data[i]);
  }
  for (map<int, Cluster *>::iterator it = sorted.begin (); it != sorted.end (); it++)
  {
	Cluster * c = it->second;
	c->index = clusters.size ();
	clusters.push_back (c);
	c->used.resize (c->support.size ());
	c->used.clear ();
  }

  // Train pairwise
  for (int i = 0; i < clusters.size (); i++)
  {
	for (int j = i + 1; j < clusters.size (); j++)
	{
	  Decision * d = new Decision;
	  decisions.push_back (d);
	  d->I = clusters[i];
	  d->J = clusters[j];
	  d->train (this);
	}
  }

  // Strip out unused support vectors
  for (int i = 0; i < decisions.size (); i++) decisions[i]->strip ();
  for (int i = 0; i < clusters .size (); i++) clusters [i]->strip ();
}

int
SVM::classify (const Vector<float> & point)
{
  MatrixPacked<float> P;
  project (point, P);
  int count = clusters.size ();
  vector<int> votes (count, 0);
  for (int k = 0; k < decisions.size (); k++)
  {
	Decision * d = decisions[k];
	int i = d->I->index;
	int j = d->J->index;
	if (P(i,j) > 0) votes[i]++;
	else            votes[j]++;
  }

  int result = 0;
  for (int i = 0; i < count; i++)
  {
	if (votes[i] > votes[result]) result = i;
  }
  return result;
}

Vector<float>
SVM::distribution (const Vector<float> & point)
{
  const double probabilityMin = 1e-7;
  const double probabilityMax = 1 - probabilityMin;

  MatrixPacked<float> P;
  project (point, P);

  const int count = clusters.size ();
  Matrix<double> R (count, count);
  R.clear ();
  for (int k = 0; k < decisions.size (); k++)
  {
	Decision * d = decisions[k];
	int i = d->I->index;
	int j = d->J->index;
	double p = d->p[0] * P(i,j) + d->p[1];
	if (p >= 0)
	{
	  double e = exp (-p);
	  p = e / (1 + e);
	}
	else
	{
	  p = 1 / (1 + exp (p));
	}
	p = max (p, probabilityMin);
	p = min (p, probabilityMax);
	R(i,j) = p;
	R(j,i) = 1 - p;
  }

  // Set up equation 21 from "Probability Estimates for Multi-Class
  // Classification by Pairwise Coupling" [Wu, Lin, Weng, 2004]
  // TODO: This method may be a bit slower than the iterative method, probably
  // because the iterative method can have a more relaxed convergence threshold.
  MatrixPacked<double> Q (count + 1);
  for (int i = 0; i < count; i++)
  {
	Q(i,i) = R.column (i).sumSquares ();
	for (int j = i + 1; j < count; j++) Q(i,j) = -R(i,j) * R(j,i);
  }
  Q.row (count).clear (1);
  Q.column (count).clear (1);
  Q(count,count) = 0;

  Vector<double> b (count + 1);
  b.clear ();
  b[count] = 1;

  Vector<double> x;
  gelss (Q, x, b);
  return x.region (0, 0, count-1, 0);  // throw away the Lagrange multiplier and convert to float
}

int
SVM::classCount ()
{
  return clusters.size ();
}

Vector<float>
SVM::representative (int group)
{
  // Note that not every support vector actually gets classfied as part of the cluster.
  if (group >= clusters.size ()) throw "Requested cluster does not exist";
  Cluster * c = clusters[group];
  int count = c->support.size ();
  for (int i = 0; i < count; i++)
  {
	if (classify (c->support[i]) == group) return c->support[i];
  }
  return c->support[0];
}

void
SVM::project (const Vector<float> & point, MatrixPacked<float> & result)
{
  int count = clusters.size ();
  vector<Vector<float> > kernel (count);
  for (int i = 0; i < count; i++)
  {
	Cluster * c = clusters[i];
	int n = c->support.size ();
	Vector<float> & v = kernel[i];
	v.resize (n);
	for (int j = 0; j < n; j++) v[j] = metric->value (point, c->support[j]);
  }

  result.resize (count, count);
  result.clear ();
  for (int k = 0; k < decisions.size (); k++)
  {
	Decision * d = decisions[k];
	int i = d->I->index;
	int j = d->J->index;
	result(i,j) = d->alphaI.dot (kernel[i]) + d->alphaJ.dot (kernel[j]) - d->rho;
  }
}

void
SVM::serialize (Archive & archive, uint32_t version)
{
  archive.registerClass<Cluster>  ("SVM Cluster");
  archive.registerClass<Decision> ("SVM Decision");
  archive.registerClass<RBF>      ("RBF");

  archive & *((ClusterMethod *) this);
  archive & clusters;
  archive & decisions;
  archive & metric;

  for (int i = 0; i < clusters.size (); i++) clusters[i]->index = i;
}


// class SVM::Cluster ---------------------------------------------------------

uint32_t SVM::Cluster::serializeVersion = 0;

void
SVM::Cluster::strip ()
{
  int p = 0;
  for (int i = 0; i < used.rows (); i++)
  {
	if (used[i])
	{
	  if (i != p) support[p] = support[i];
	  p++;
	}
  }
  support.resize (p);
}

void
SVM::Cluster::serialize (Archive & archive, uint32_t version)
{
  archive & support;
}


// class SVM::Train -----------------------------------------------------------

SVM::Train::Train (int index, Vector<float> & x, float y)
: index (index),
  x (&x),
  y (y)
{
  computed = false;
  alpha = 0;
  p = -1;
  g = p;
}


// class SigmoidFunction ------------------------------------------------------

/**
   Encapsulates the problem of computing a linear transformation for
   decisions values before they are passed through a sigmoid function to
   produce a decision probability.
   Utility class used by SVM::Decision::train().
 **/
class SigmoidFunction : public fl::SearchableNumeric<float>
{
public:
  SigmoidFunction (vector<SVM::Train *> & trainset, Vector<float> & f)
  : t (f.rows ()),
	f (f),
	atValue (2),
	atGradient (2),
	g (2),
	h (2, 2)
  {
	atValue   .clear (NAN);
	atGradient.clear (NAN);

	int positive = 0;
	int negative = 0;
	dim = f.rows ();
	for (int i = 0; i < dim; i++)
	{
	  if (trainset[i]->y > 0) positive++;
	  else                    negative++;
	}
	float hi = (positive + 1.0f) / (positive + 2.0f);
	float lo = 1.0f / (negative + 2);
	B = log ((negative + 1.0f) / (positive + 1.0f));
	for (int i = 0; i < dim; i++) t[i] = (trainset[i]->y > 0) ? hi : lo;
  }
  virtual Search<float> * search ()
  {
	return new LevenbergMarquardt<float>;
  }
  virtual MatrixResult<float> start ()
  {
	Vector<float> * result = new Vector<float> (2);
	(*result)[0] = 0;
	(*result)[1] = B;
	return result;
  }
  virtual int dimension (const Vector<float> & point)
  {
	return dim;
  }
  virtual MatrixResult<float> value (const Vector<float> & point)
  {
	computeValue (point);
	Vector<float> * result = new Vector<float> (dim);
	for (int i = 0; i < dim; i++)
	{
	  float & a = fApB[i];
	  if (a >= 0) (*result)[i] =  t[i]      * a + log (1 + exp (-a));
	  else        (*result)[i] = (t[i] - 1) * a + log (1 + exp ( a));
	}
	cerr << "point = " << point << " " << result->norm (1) << endl;
	return result;
  }
  virtual MatrixResult<float> gradient (const Vector<float> & point, const Vector<float> * currentValue = NULL)
  {
	computeGradient (point);
	Vector<float> * result = new Vector<float>;
	result->copyFrom (g);
	return result;
  }
  virtual MatrixResult<float> hessian (const Vector<float> & point, const Vector<float> * currentValue = NULL)
  {
	computeGradient (point);
	Matrix<float> * result = new Matrix<float>;
	result->copyFrom (h);
	return result;
  }
  void computeValue (const Vector<float> & point)
  {
	if (atValue != point)
	{
	  atValue.copyFrom (point);
	  fApB = f * point[0] + point[1];
	}
  }
  void computeGradient (const Vector<float> & point)
  {
	computeValue (point);
	if (atGradient != point)
	{
	  atGradient.copyFrom (point);
	  g.clear ();
	  h.clear (1e-12);
	  float & g0 = g[0];
	  float & g1 = g[1];
	  float & h00 = h(0,0);
	  float & h11 = h(1,1);
	  float & h01 = h(0,1);
	  for (int i = 0; i < dim; i++)
	  {
		float & a = fApB[i];
		float & fi = f[i];
		float p;
		float q;
		if (a >= 0)
		{
		  p = exp (-a) / (1.0f + exp (-a));
		  q = 1.0f / (1.0f + exp (-a));
		}
		else
		{
		  p = 1.0f / (1.0f + exp (a));
		  q = exp (a) / (1.0f + exp (a));
		}
		float pq = p * q;
		h00 += fi * fi * pq;
		h01 +=      fi * pq;
		h11 +=           pq;
		float tp = t[i] - p;
		g0 += fi * tp;
		g1 +=      tp;
	  }
	  h(1,0) = h01;
	}
  }
  int dim;
  float B;
  Vector<float> t;
  Vector<float> f;
  Vector<float> atValue;
  Vector<float> atGradient;
  Vector<float> fApB;
  Vector<float> g;
  Matrix<float> h;
};


// class SVM::Decision --------------------------------------------------------

uint32_t SVM::Decision::serializeVersion = 0;
float SVM::Decision::tau = 1e-6;

/**
   Implements the algorithm given in the appendix of "Working Set Selection
   Using Second Order Information for Training Support Vector Machines"
   by Fan, Chen and Lin, 2005.  (Which, by the way, is a very lucid account
   of what we are actually trying to optimize.)
 **/
void
SVM::Decision::train (SVM * svm)
{
  // Initialize structures
  this->svm = svm;
  const int countI = I->support.size ();
  const int countJ = J->support.size ();
  const int total  = countI + countJ;
  trainset.resize (total);
  for (int i = 0;      i < countI; i++) trainset[i] = new Train (i, I->support[i],         1);
  for (int i = countI; i < total;  i++) trainset[i] = new Train (i, J->support[i-countI], -1);
  Q.resize (total);
  Q.clear (NAN);
  for (int i = 0; i < total; i++)
  {
	Train * a = trainset[i];
	Q(i,i) = svm->metric->value (*a->x, *a->x);
  }

  // Optimization loop
  int maxIterations = max (10000000ll, 100ll * total);
  int maxWait = min (total, 1000);
  int iteration = 0;
  while (iteration++ < maxIterations)
  {
	if (iteration % maxWait == 0) cerr << ".";

	Train * i;
	Train * j;
	if (selectWorkingSet (i, j) < svm->epsilon) break;

	computeColumn (i);
	computeColumn (j);

	float deltaI = i->alpha;
	float deltaJ = j->alpha;

	if (i->y != j->y)
	{
	  float a = Q(i->index,i->index) + Q(j->index,j->index) + 2 * Q(i->index,j->index);
	  a = max (a, tau);
	  float delta = (-i->g - j->g) / a;
	  float diff = i->alpha - j->alpha;
	  i->alpha += delta;
	  j->alpha += delta;

	  if (diff > 0)
	  {
		if (j->alpha < 0)
		{
		  j->alpha = 0;
		  i->alpha = diff;
		}
		if (i->alpha > 1)
		{
		  i->alpha = 1;
		  j->alpha = 1 - diff;
		}
	  }
	  else  // diff <= 0
	  {
		if (i->alpha < 0)
		{
		  i->alpha = 0;
		  j->alpha = -diff;
		}
		if (j->alpha > 1)
		{
		  j->alpha = 1;
		  i->alpha = 1 + diff;
		}
	  }
	}
	else  // i->y == j->y
	{
	  float a = Q(i->index,i->index) + Q(j->index,j->index) - 2 * Q(i->index,j->index);
	  a = max (a, tau);
	  float delta = (i->g - j->g) / a;
	  float sum = i->alpha + j->alpha;
	  i->alpha -= delta;
	  j->alpha += delta;

	  if (sum > 1)
	  {
		if (i->alpha > 1)
		{
		  i->alpha = 1;
		  j->alpha = sum - 1;
		}
		if (j->alpha > 1)
		{
		  j->alpha = 1;
		  i->alpha = sum - 1;
		}
	  }
	  else  // sum <= 1
	  {
		if (j->alpha < 0)
		{
		  j->alpha = 0;
		  i->alpha = sum;
		}
		if (i->alpha < 0)
		{
		  i->alpha = 0;
		  j->alpha = sum;
		}
	  }
	}

	deltaI = i->alpha - deltaI;
	deltaJ = j->alpha - deltaJ;
	for (int k = 0; k < trainset.size (); k++)
	{
	  Train * t = trainset[k];
	  t->g += Q(i->index,t->index) * deltaI + Q(j->index,t->index) * deltaJ;
	}
  }

  // Save the solution
  alphaI.resize (countI);
  alphaJ.resize (countJ);
  for (int i = 0; i < total; i++)
  {
	Train * t = trainset[i];
	if (t->index < countI) alphaI[t->index       ] = t->alpha * t->y;
	else                   alphaJ[t->index-countI] = t->alpha * t->y;
  }
  for (int i = 0; i < countI; i++) if (alphaI[i]) I->used[i] = true;
  for (int i = 0; i < countJ; i++) if (alphaJ[i]) J->used[i] = true;

  // Determine rho
  float freeTotal = 0;
  int freeCount = 0;
  float hi =  INFINITY;
  float lo = -INFINITY;
  for (int i = 0; i < total; i++)
  {
	Train * t = trainset[i];
	float yg = t->y * t->g;

	if (t->alpha >= 1)  // at upper bound
	{
	  if (t->y > 0) lo = max (lo, yg);
	  else          hi = min (hi, yg);
	}
	else if (t->alpha <= 0)  // at lower bound
	{
	  if (t->y > 0) hi = min (hi, yg);
	  else          lo = max (lo, yg);
	}
	else  // free
	{
	  freeCount++;
	  freeTotal += yg;
	}
  }
  if (freeCount) rho = freeTotal / freeCount;
  else           rho = (hi + lo) / 2;

  // Determine probability coefficients
  // Uses method in "A Note on Platt's Probabilistic Outputs for Support
  // Vector Machines" by Lin and Weng
  //   Calculate decision values
  Vector<float> f (total);
  f.clear ();
  for (int i = 0; i < total; i++)
  {
	Train * a = trainset[i];
	if (! a->alpha) continue;
	for (int j = 0; j < total; j++)
	{
	  Train * b = trainset[j];
	  f[b->index] += Q(a->index,b->index) * a->alpha * b->y;  // multiply Q(a,b) by a->y and b->y to factor out the sign that was included above
	}
  }
  f -= rho;
  //   Solve coefficients for sigmoid function
  SigmoidFunction problem (trainset, f);
  Search<float> * solver = problem.search ();
  p = problem.start ();
  solver->search (problem, p);
  delete solver;

  // Destroy trainset
  for (int i = 0; i < trainset.size (); i++) delete trainset[i];
  trainset.clear ();
  Q.detach ();
}

float
SVM::Decision::selectWorkingSet (Train * & i, Train * & j)
{
  float Gmax  = -INFINITY;
  for (int k = 0; k < trainset.size (); k++)
  {
	Train * t = trainset[k];
	if (t->y > 0)  // t->y == +1
	{
	  if (t->alpha < 1  &&  -t->g >= Gmax)
	  {
		Gmax = -t->g;
		i = t;
	  }
	}
	else  // t->y == -1
	{
	  if (t->alpha > 0  &&  t->g >= Gmax)
	  {
		Gmax = t->g;
		i = t;
	  }
	}
  }

  computeColumn (i);
  float Gmax2 = -INFINITY;
  float Omin = INFINITY;
  for (int k = 0; k < trainset.size (); k++)
  {
	Train * t = trainset[k];
	if (t->y > 0)  // t->y == +1
	{
	  if (t->alpha > 0)
	  {
		float g = Gmax + t->g;
		Gmax2 = max (Gmax2, t->g);
		if (g > 0)
		{
		  float o;
		  float a = Q(i->index,i->index) + Q(t->index,t->index) - 2 * i->y * Q(i->index,t->index);
		  if (a > 0) o = -g * g / a;
		  else       o = -g * g / tau;
		  if (o <= Omin)
		  {
			j = t;
			Omin = o;
		  }
		}
	  }
	}
	else  // t->y == -1
	{
	  if (t->alpha < 1)
	  {
		float g = Gmax - t->g;
		Gmax2 = max (Gmax2, -t->g);
		if (g > 0)
		{
		  float o;
		  float a = Q(i->index,i->index) + Q(t->index,t->index) - 2 * i->y * Q(i->index,t->index);
		  if (a > 0) o = -g * g / a;
		  else       o = -g * g / tau;
		  if (o <= Omin)
		  {
			j = t;
			Omin = o;
		  }
		}
	  }
	}
  }

  return Gmax + Gmax2;
}

void
SVM::Decision::computeColumn (Train * i)
{
  if (i->computed) return;
  i->computed = true;

  for (int k = 0; k < trainset.size (); k++)
  {
	Train * j = trainset[k];
	float & q = Q(i->index,j->index);
	if (isnan (q)) q = i->y * j->y * svm->metric->value (*i->x, *j->x);
  }
}

void
SVM::Decision::strip ()
{
  int p = 0;
  for (int i = 0; i < I->used.rows (); i++)
  {
	if (I->used[i])
	{
	  if (i != p) alphaI[p] = alphaI[i];
	  p++;
	}
  }
  alphaI.resize (p);

  p = 0;
  for (int i = 0; i < J->used.rows (); i++)
  {
	if (J->used[i])
	{
	  if (i != p) alphaJ[p] = alphaJ[i];
	  p++;
	}
  }
  alphaJ.resize (p);
}

void
SVM::Decision::serialize (Archive & archive, uint32_t version)
{
  archive & I;
  archive & J;
  archive & alphaI;
  archive & alphaJ;
  archive & rho;
  archive & p;
}
