// Similar to LevenbergMarquardtSparseBK, but uses sparse Cholesky
// decomposition instead.  Works OK, but not as numerically stable as BK,
// and not significantly more efficient either.


#include "fl/search.h"

#include <float.h>
#include <algorithm>


// For debugging
//#include <iostream>
#include <fstream>


using namespace fl;
using namespace std;


// Support functions ----------------------------------------------------------

class SparseCholesky : public MatrixSparse<double>
{
public:
  SparseCholesky () : MatrixSparse<double> () {}
  SparseCholesky (int rows, int columns) : MatrixSparse<double> (rows, columns) {}

  double dot (const int column, const int lastRow, const Vector<double> & x) const
  {
	double result = 0;
	map<int,double> & C = (*data)[column];
	map<int,double>::iterator i = C.begin ();
	while (i->first <= lastRow  &&  i != C.end ())
	{
	  result += x[i->first] * i->second;
	  i++;
	}
	return result;
  }

  // Return the upper triangular part of ~this * this in a new matrix.
  SparseCholesky transposeSquare () const
  {
	const int n = data->size ();
	SparseCholesky result (n, n);

	for (int c = 0; c < n; c++)
	{
	  map<int,double> & C = (*result.data)[c];
	  map<int,double>::iterator i = C.begin ();
	  for (int r = 0; r <= c; r++)
	  {
		double t = 0;
		map<int,double> & C1 = (*data)[r];
		map<int,double> & C2 = (*data)[c];
		map<int,double>::iterator i1 = C1.begin ();
		map<int,double>::iterator i2 = C2.begin ();
		while (i1 != C1.end ()  &&  i2 != C2.end ())
		{
		  if (i1->first == i2->first)
		  {
			t += i1->second * i2->second;
			i1++;
			i2++;
		  }
		  else if (i1->first > i2->first)
		  {
			i2++;
		  }
		  else  // i1->first < i2->first
		  {
			i1++;
		  }
		}
		if (t != 0)
		{
		  i = C.insert (i, make_pair (r, t));
		}
	  }
	}

	return result;
  }

  Vector<double> transposeMult (const Vector<double> & x) const
  {
	const int n = data->size ();
	Vector<double> result (n);

	for (int c = 0; c < n; c++)
	{
	  result[c] = 0;
	  map<int,double> & C = (*data)[c];
	  map<int,double>::iterator i = C.begin ();
	  while (i != C.end ())
	  {
		result[c] += x[i->first] * i->second;
		i++;
	  }
	}

	return result;
  }

  Vector<double> operator * (const Vector<double> & x) const
  {
	const int n = data->size ();

	Vector<double> result (rows_);
	result.clear ();

	for (int c = 0; c < n; c++)
	{
	  map<int,double> & C = (*data)[c];
	  map<int,double>::iterator i = C.begin ();
	  while (i != C.end ())
	  {
		result[i->first] += i->second * x[c];
		i++;
	  }
	}

	return result;
  }

  Vector<double> trimult (const Vector<double> & x) const
  {
	const int n = data->size ();

	Vector<double> result (rows_);
	result.clear ();

	for (int c = 0; c < n; c++)
	{
	  map<int,double> & C = (*data)[c];
	  map<int,double>::iterator i = C.begin ();
	  while (i != C.end ())
	  {
		result[i->first] += i->second * x[c];
		if (i->first < c)
		{
		  result[c] += i->second * x[i->first];
		}
		i++;
	  }
	}

	return result;
  }

  void addDiagonal (const double alpha, const Vector<double> & x)
  {
	const int n = data->size ();

	for (int j = 0; j < n; j++)
	{
	  double value = alpha * x[j] * x[j];

	  map<int,double> & C = (*data)[j];
	  map<int,double>::reverse_iterator i = C.rbegin ();
	  while (i->first > j)  // This should never happen.
	  {
		i++;
	  }
	  if (i->first == j)
	  {
		i->second += value;
	  }
	  else  // This also should never happen.
	  {
		C.insert (i.base (), make_pair (j, value));
	  }
	}
  }

  double frob2 (const int column) const
  {
	double result = 0;
	map<int,double> & C = (*data)[column];
	map<int,double>::iterator i = C.begin ();
	while (i != C.end ())
	{
	  result += i->second * i->second;
	  i++;
	}
	return sqrt (result);
  }

  void factorize ()
  {
	const int n = data->size ();

	for (int c = 0; c < n; c++)
	{
	  map<int,double> & Cc = (*data)[c];
	  map<int,double>::iterator i = Cc.begin ();

	  for (int r = i->first; r < c; r++)
	  {
		double a;
		if (i->first == r)
		{
		  a = i->second;
		}
		else
		{
		  a = 0;
		}

		map<int,double> & Cr = (*data)[r];
		map<int,double>::iterator kc = Cc.begin ();
		map<int,double>::iterator kr = Cr.begin ();
		while (kc != Cc.end ()  &&  kr != Cr.end ()  &&  kc->first < r  &&  kr->first < r)
		{
		  if (kc->first == kr->first)
		  {
			a -= kc->second * kr->second;
			kc++;
			kr++;
		  }
		  else if (kc->first > kr->first)
		  {
			kr++;
		  }
		  else  // kc->first < kr->first
		  {
			kc++;
		  }
		}

		a /= Cr.rbegin ()->second;  // This is safe because already guarded by r == c case below.

		// Store result and advance pointer
		if (a != 0)
		{
		  if (i->first == r)
		  {
			i->second = a;
			i++;
		  }
		  else  // element does not exist
		  {
			Cc.insert (i, make_pair (r, a));
		  }
		}
		else if (i->first == r) // Need to erase element
		{
		  map<int,double>::iterator temp = i;
		  i++;
		  Cc.erase (temp);
		}
	  }

	  // Special case for r == c
	  map<int,double>::reverse_iterator diag = Cc.rbegin ();
	  if (diag->first != c)
	  {
		throw "SparseCholesky: matrix not upper triangular or missing diagonal element";
	  }
	  double a = diag->second;
	  i = Cc.begin ();
	  while (i != Cc.end ()  &&  i->first < c)
	  {
		a -= i->second * i->second;
		i++;
	  }
	  if (a <= 0)
	  {
		cerr << "SparseCholesky: a is non-positive: " << a << " " << c << " " << n << " " << Cc.size () << endl;
		//throw "SparseCholesky: not positive definite";
		a = 1e-1;
	  }
	  diag->second = sqrt (a);
	}
  }

  void solve (Vector<double> & x, const Vector<double> & b) const
  {
	const int n = b.rows ();
	x.resize (n);

	// Forward substitution
	for (int i = 0; i < n; i++)
	{
	  x[i] = (b[i] - dot (i, i - 1, x)) / (*data)[i].rbegin ()->second;
	}

	// Back substitution
	for (int i = n - 1; i >= 0; i--)
	{
	  x[i] /= (*data)[i].rbegin ()->second;

	  map<int,double> & C = (*data)[i];
	  map<int,double>::iterator k = C.begin ();
	  while (k->first < i  &&  k != C.end ())
	  {
		x[k->first] -= x[i] * k->second;
		k++;
	  }
	}
  }
};

static void
lmpar (const SparseCholesky & fjac, const Vector<double> & diag, const Vector<double> & fvec, double delta, double & par, Vector<double> & x)
{
  int n = fjac.columns ();

  // Compute and store in x the gauss-newton direction.
  // ~fjac * fjac * x = ~fjac * fvec
  Vector<double> Jf = fjac.transposeMult (fvec);
  SparseCholesky JJ = fjac.transposeSquare ();
  SparseCholesky factoredJJ;
  factoredJJ.copyFrom (JJ);
  factoredJJ.factorize ();
  factoredJJ.solve (x, Jf);
cerr << "qos1 = " << (JJ.trimult (x) - Jf).frob (2) << endl;

  // Evaluate the function at the origin, and test
  // for acceptance of the gauss-newton direction.
  Vector<double> dx (n);
  for (int j = 0; j < n; j++)
  {
	dx[j] = diag[j] * x[j];
  }
  double dxnorm = dx.frob (2);
  double fp = dxnorm - delta;
  if (fp <= 0.1 * delta)
  {
	par = 0;
	return;
  }

  // The jacobian is required to have full rank, so the newton
  // step provides a lower bound, parl, for the zero of
  // the function.
  Vector<double> wa1 (n);
  for (int j = 0; j < n; j++)
  {
	wa1[j] = diag[j] * dx[j] / dxnorm;
  }
  Vector<double> wa2;
  factoredJJ.solve (wa2, wa1);
  double parl = fp / (delta * wa1.dot (wa2));

  // Calculate an upper bound, paru, for the zero of the function.
  for (int j = 0; j < n; j++)
  {
	wa1[j] = Jf[j] / diag[j];
  }
  double gnorm = wa1.frob (2);
  double paru = gnorm / delta;
  if (paru == 0)
  {
	paru = DBL_MIN / min (delta, 0.1);
  }

  // If the input par lies outside of the interval (parl,paru),
  // set par to the closer endpoint.
  par = max (par, parl);
  par = min (par, paru);
  if (par == 0)
  {
	par = gnorm / dxnorm;
  }

  int iter = 0;
  while (true)
  {
	iter++;

	// Evaluate the function at the current value of par.
	if (par == 0)
	{
	  par = max (DBL_MIN, 0.001 * paru);
	}
	factoredJJ.copyFrom (JJ);
	factoredJJ.addDiagonal (par, diag);
SparseCholesky tJJ;
tJJ.copyFrom (factoredJJ);
	factoredJJ.factorize ();
	factoredJJ.solve (x, Jf);
cerr << "qos2 = " << (tJJ.trimult (x) - Jf).frob (2) << endl;

	for (int j = 0; j < n; j++)
	{
	  dx[j] = diag[j] * x[j];
	}
	dxnorm = dx.frob (2);
	double oldFp = fp;
	fp = dxnorm - delta;

	// If the function is small enough, accept the current value
	// of par.  Also test for the exceptional cases where parl
	// is zero or the number of iterations has reached 10.
	if (   fabs (fp) <= 0.1 * delta
	    || (parl == 0  &&  fp <= oldFp  &&  oldFp < 0)
	    || iter >= 10)
	{
	  return;
	}

	// Compute the newton correction.
	for (int j = 0; j < n; j++)
	{
	  wa1[j] = diag[j] * dx[j] / dxnorm;
	}
	factoredJJ.solve (wa2, wa1);
	double parc = fp / (delta * wa1.dot (wa2));

	// Depending on the sign of the function, update parl or paru.
	if (fp > 0)
	{
	  parl = max (parl, par);
	}
	if (fp < 0)
	{
	  paru = min (paru, par);
	}
	// Compute an improved estimate for par.
	par = max (parl, par + parc);
  }
}


// class LevenbergMarquardtSparseCholesky -------------------------------------

LevenbergMarquardtSparseCholesky::LevenbergMarquardtSparseCholesky (double toleranceF, double toleranceX, int maxIterations)
{
  if (toleranceF < 0)
  {
	toleranceF = sqrt (DBL_EPSILON);
  }
  this->toleranceF = toleranceF;

  if (toleranceX < 0)
  {
	toleranceX = sqrt (DBL_EPSILON);
  }
  this->toleranceX = toleranceX;

  this->maxIterations = maxIterations;
}

void
LevenbergMarquardtSparseCholesky::search (Searchable & searchable, Vector<double> & point)
{
  // The following is a loose paraphrase the MINPACK function lmdif

  const double toleranceG = 0;

  // Evaluate the function at the starting point and calculate its norm.
  Vector<double> fvec;
  searchable.value (point, fvec);

  int m = fvec.rows ();
  int n = point.rows ();

  SparseCholesky fjac (m, n);
  Vector<double> diag (n);  // scales
  double par = 0;  // levenberg-marquardt parameter
  double fnorm = fvec.frob (2);
  double xnorm;
  double delta;

  // outer loop
  int iter = 1;
  while (true)
  {
	// calculate the jacobian matrix
	searchable.jacobian (point, fjac, &fvec);

	// compute the qr factorization of the jacobian.
	Vector<double> jacobianNorms (n);
	for (int j = 0; j < n; j++)
	{
	  jacobianNorms[j] = fjac.frob2 (j);
	}

	// On the first iteration ...
	if (iter == 1)
	{
	  // Scale according to the norms of the columns of the initial jacobian.
	  for (int j = 0; j < n; j++)
	  {
		diag[j] = jacobianNorms[j];
		if (diag[j] == 0)
		{
		  diag[j] = 1;
		}
	  }

	  // Calculate the norm of the scaled x and initialize the step bound delta.
	  xnorm = 0;
	  for (int j = 0; j < n; j++)
	  {
		double temp = diag[j] * point[j];
		xnorm += temp * temp;
	  }
	  xnorm = sqrt (xnorm);

	  const double factor = 1;
	  delta = factor * xnorm;
	  if (delta == 0)
	  {
		delta = factor;
	  }
	}

	// compute the norm of the scaled gradient
	double gnorm = 0;
	if (fnorm != 0)
	{
	  for (int j = 0; j < n; j++)
	  {
		if (jacobianNorms[j] != 0)
		{
		  double value = fjac.dot (j, m - 1, fvec);
		  gnorm = max (gnorm, fabs (value / (fnorm * jacobianNorms[j])));
		}
	  }
	}

	// test for convergence of the gradient norm
	if (gnorm <= toleranceG)
	{
	  //info = 4;
	  return;
	}

	// rescale if necessary
	for (int j = 0; j < n; j++)
	{
	  diag[j] = max (diag[j], jacobianNorms[j]);
	}

	// beginning of the inner loop
	double ratio = 0;
	while (ratio < 0.0001)
	{
	  // determine the levenberg-marquardt parameter.
	  Vector<double> p (n);  // wa1
	  lmpar (fjac, diag, fvec, delta, par, p);

	  // store the direction p and x + p. calculate the norm of p.
	  p *= -1;
	  Vector<double> xp = point + p;
	  double pnorm = 0;
	  for (int j = 0; j < n; j++)
	  {
		double temp = diag[j] * p[j];
		pnorm += temp * temp;
	  }
	  pnorm = sqrt (pnorm);

	  // on the first iteration, adjust the initial step bound
	  if (iter == 1)
	  {
		delta = min (delta, pnorm);
	  }

	  // evaluate the function at x + p and calculate its norm
	  Vector<double> tempFvec;
	  searchable.value (xp, tempFvec);
	  double fnorm1 = tempFvec.frob (2);

	  // compute the scaled actual reduction
	  double actred = -1;
	  if (0.1 * fnorm1 < fnorm)
	  {
		double temp = fnorm1 / fnorm;
		actred = 1 - temp * temp;
	  }

	  // compute the scaled predicted reduction and the scaled directional derivative
	  double temp1 = (fjac * p).frob (2) / fnorm;
	  double temp2 = sqrt (par) * pnorm / fnorm;
	  double prered = temp1 * temp1 + temp2 * temp2 / 0.5;
	  double dirder = -(temp1 * temp1 + temp2 * temp2);

	  // compute the ratio of the actual to the predicted reduction
	  ratio = 0;
	  if (prered != 0)
	  {
		ratio = actred / prered;
	  }

	  // update the step bound
	  if (ratio <= 0.25)
	  {
		double temp;
		if (actred >= 0)
		{
		  temp = 0.5;
		}
		else
		{
		  temp = 0.5 * dirder / (dirder + 0.5 * actred);
		}
		if (0.1 * fnorm1 >= fnorm  ||  temp < 0.1)
		{
		  temp = 0.1;
		}
		delta = temp * min (delta, pnorm / 0.1);
		par /= temp;
	  }
	  else
	  {
		if (par == 0  ||  ratio >= 0.75)
		{
		  delta = pnorm / 0.5;
		  par = 0.5 * par;
		}
	  }

	  // test for successful iteration.
	  if (ratio >= 0.0001)
	  {
	    // successful iteration. update x, fvec, and their norms.
		point = xp;
		fvec = tempFvec;

		xnorm = 0;
		for (int j = 0; j < n; j++)
		{
		  double temp = diag[j] * point[j];
		  xnorm += temp * temp;
		}
		xnorm = sqrt (xnorm);

		fnorm = fnorm1;
		iter++;
	  }

	  // tests for convergence
	  if (   fabs (actred) <= toleranceF
		  && prered <= toleranceF
		  && 0.5 * ratio <= 1)
	  {
		// info = 1;
		return;
	  }
	  if (delta <= toleranceX * xnorm)
	  {
		// info = 2;
		return;
	  }

	  // tests for termination and stringent tolerances
	  if (iter > maxIterations)
	  {
		throw (int) 5;
	  }
	  if (   fabs (actred) <= DBL_EPSILON
		  && prered <= DBL_EPSILON
		  && 0.5 * ratio <= 1)
	  {
		throw (int) 6;
	  }
	  if (delta <= DBL_EPSILON * xnorm)
	  {
		throw (int) 7;
	  }
	  if (gnorm <= DBL_EPSILON)
	  {
		throw (int) 8;
	  }
	}
  }
}
