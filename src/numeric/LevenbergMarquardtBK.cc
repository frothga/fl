// This is a snapshot of the code just after converting to triangular
// factorization (rather than QR), but before sparsification.  It worked
// perfectly, at least for small problems, and the numbers from various parts
// of the algorithm exactly matched those output by the C-ified but unmodified
// LM.


#include "fl/search.h"

#include <float.h>


// For debugging
//#include <iostream>


using namespace fl;
using namespace std;


// Support functions ----------------------------------------------------------

extern "C"
{
  void dsytf2_ (const char & uplo,
				const int & N,
				double A[],
				const int & LDA,
				int IPIV[],
				int & INFO);

  void dsytrs_ (const char & uplo,
				const int & N,
				const int & NRHS,
				const double A[],
				const int & LDA,
				const int IPIV[],
				double B[],
				const int & LDB,
				int & INFO);
}

static void
lmpar (const Matrix<double> & fjac, const Vector<double> & diag, const Vector<double> & fvec, double delta, double & par, Vector<double> & x)
{
  int n = fjac.columns ();

  // Compute and store in x the gauss-newton direction.
  // ~fjac * fjac * x = ~fjac * fvec
  Matrix<double> Jf = ~fjac * fvec;
  Matrix<double> JJ = ~fjac * fjac;
  Matrix<double> factoredJJ;
  factoredJJ.copyFrom (JJ);
  Vector<int> ipvt (n);
  int info;
  dsytf2_ ('U', n, &factoredJJ(0,0), n, &ipvt[0], info);
  x.copyFrom (Jf);
  dsytrs_ ('U', n, 1, &factoredJJ(0,0), n, &ipvt[0], &x[0], n, info);

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
  wa2.copyFrom (wa1);
  dsytrs_ ('U', n, 1, &factoredJJ(0,0), n, &ipvt[0], &wa2[0], n, info);
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
	for (int j = 0; j < n; j++)
	{
	  factoredJJ(j,j) += par * diag[j] * diag[j];
	}
	dsytf2_ ('U', n, &factoredJJ(0,0), n, &ipvt[0], info);
	x.copyFrom (Jf);
	dsytrs_ ('U', n, 1, &factoredJJ(0,0), n, &ipvt[0], &x[0], n, info);

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
	Vector<double> wa2;
	wa2.copyFrom (wa1);
	dsytrs_ ('U', n, 1, &factoredJJ(0,0), n, &ipvt[0], &wa2[0], n, info);
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


// class LevenbergMarquardtSparse ---------------------------------------------

LevenbergMarquardtSparse::LevenbergMarquardtSparse (double toleranceF, double toleranceX, int maxIterations)
{
  this->maxIterations = maxIterations;

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
}

void
LevenbergMarquardtSparse::search (Searchable & searchable, Vector<double> & point)
{
  // The following is a loose paraphrase the MINPACK function lmdif

  const double toleranceG = 0;

  // Evaluate the function at the starting point and calculate its norm.
  Vector<double> fvec;
  searchable.value (point, fvec);

  int m = fvec.rows ();
  int n = point.rows ();

  Matrix<double> fjac (m, n);
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
	  jacobianNorms[j] = fjac.column (j).frob (2);
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
		  double value = fjac.column (j).dot (fvec);
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
		  double temp = diag[j] + point[j];
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
