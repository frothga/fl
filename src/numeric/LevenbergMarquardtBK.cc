#include "fl/search.h"

#include <float.h>


// For debugging
//#include <iostream>


using namespace fl;
using namespace std;


// Support functions ----------------------------------------------------------

static void
factorize (Matrix<double> & A, Vector<int> & pivots)
{
  // Factorize A as U*D*U' using the upper triangle of A

  const double alpha = (1.0 + sqrt (17.0)) / 8.0;
  int n = A.columns ();

  pivots.resize (n);

  // K is the main loop index, decreasing from N to 1 in steps of
  // 1 or 2
  int k = n - 1;
  while (k >= 0)
  {
	// Determine rows and columns to be interchanged and whether
	// a 1-by-1 or 2-by-2 pivot block will be used

	int kstep = 1;

	double absakk = fabs (A(k,k));

	// IMAX is the row-index of the largest off-diagonal element in
	// column K, and COLMAX is its absolute value
	int imax = 0;
	double colmax = 0;
	for (int i = 0; i < k; i++)
	{
	  double value = fabs (A(i,k));
	  if (value > colmax)
	  {
		imax = i;
		colmax = value;
	  }
	}

	int kp;
	if (max (absakk, colmax) == 0)
	{
	  throw -k;
	  //kp = k;
	}
	else
	{
	  if (absakk >= alpha * colmax)
	  {
		// no interchange, use 1-by-1 pivot block
		kp = k;
	  }
	  else
	  {
		// JMAX is the column-index of the largest off-diagonal
		// element in row IMAX, and ROWMAX is its absolute value
		// (jmax isn't used in this C++ adaptation)
		double rowmax = 0;
		for (int j = imax + 1; j <= k; j++)
		{
		  rowmax = max (rowmax, fabs (A(imax,j)));
		}
		for (int j = 0; j < imax; j++)
		{
		  rowmax = max (rowmax, fabs (A(j,imax)));
		}

		if (absakk >= alpha * colmax * colmax / rowmax)
		{
		  // no interchange, use 1-by-1 pivot block
		  kp = k;
		}
		else if (fabs (A(imax,imax)) >= alpha * rowmax)
		{
		  // interchange rows and columns K and IMAX, use 1-by-1 pivot block
		  kp = imax;
		}
		else
		{
		  // interchange rows and columns K-1 and IMAX, use 2-by-2 pivot block
		  kp = imax;
		  kstep = 2;
		}
	  }

	  int kk = k - kstep + 1;
	  if (kp != kk)  // then kp < kk
	  {
		// Interchange rows and columns KK and KP in the leading
		// submatrix A(1:k,1:k)
		for (int j = 0; j < kp; j++)
		{
		  swap (A(j,kk), A(j,kp));
		}
		for (int j = kp + 1; j < kk; j++)
		{
		  swap (A(j,kk), A(kp,j));
		}
		swap (A(kk,kk), A(kp,kp));
		if (kstep == 2)
		{
		  swap (A(k-1,k), A(kp,k));
		}
	  }

	  // Update the leading submatrix
	  if (kstep == 1)
	  {
		// 1-by-1 pivot block D(k): column k now holds W(k) = U(k)*D(k)
		// where U(k) is the k-th column of U

		// Perform a rank-1 update of A(1:k-1,1:k-1) as
		// A := A - U(k)*D(k)*U(k)' = A - W(k)*1/D(k)*W(k)'
		double dk = A(k,k);
		for (int j = 0; j < k; j++)
		{
		  if (A(j,k) != 0)
		  {
			double temp = -A(j,k) / dk;
			for (int i = 0; i <= j; i++)
			{
			  A(i,j) += A(i,k) * temp;
			}
		  }
		}

		// Store U(k) in column k
		MatrixRegion<double> (A, 0, k, k-1, k) /= dk;
	  }
	  else
	  {
		// 2-by-2 pivot block D(k): columns k and k-1 now hold
		// ( W(k-1) W(k) ) = ( U(k-1) U(k) )*D(k)
		// where U(k) and U(k-1) are the k-th and (k-1)-th columns of U

		// Perform a rank-2 update of A(1:k-2,1:k-2) as
		// A := A - ( U(k-1) U(k) )*D(k)*( U(k-1) U(k) )'
		//    = A - ( W(k-1) W(k) )*inv(D(k))*( W(k-1) W(k) )'
		double d12 = A(k-1,k);
		double d22 = A(k-1,k-1);
		double d11 = A(k,k);
		double temp = d12;
		d12 = d11 * d22 / d12 - d12;
		d22 /= temp;
		d11 /= temp;

		for (int j = k - 2; j >= 0; j--)
		{
		  double wkm1 = (d11 * A(j,k-1) - A(j,k))   / d12;
		  double wk   = (d22 * A(j,k)   - A(j,k-1)) / d12;
		  for (int i = j; i >= 0; i--)
		  {
			A(i,j) -= A(i,k) * wk + A(i,k-1) * wkm1;
		  }
		  A(j,k)   = wk;
		  A(j,k-1) = wkm1;
		}
	  }
	}

	// Store details of the interchanges in IPIV
	// Pivot values must be one-based so that negation can work.  The output
	// of this routine is therefore compatible with dsytf2, etc.
	kp++;
	if (kstep == 1)
	{
	  pivots[k] = kp;
	}
	else
	{
	  pivots[k] = -kp;
	  pivots[k-1] = -kp;
	}

	// Decrease K
	k -= kstep;
  }
}

static void
solve (const Matrix<double> & A, const Vector<int> & pivots, Vector<double> & x, const Vector<double> & b)
{
  // Solve A*X = B, where A = U*D*U'.

  int n = A.columns ();

  x.copyFrom (b);

  // First solve U*D*X = B
  // K is the main loop index, decreasing from N-1 to 0 in steps of
  // 1 or 2, depending on the size of the diagonal blocks.
  int k = n - 1;
  while (k >= 0)
  {
	if (pivots[k] > 0)
	{
	  // 1 x 1 diagonal block

	  // Interchange rows K and IPIV(K).
	  int kp = pivots[k] - 1;
	  if (kp != k)
	  {
		swap (x[k], x[kp]);
	  }

	  // Multiply by inv(U(K)), where U(K) is the transformation
	  // stored in column K of A.
	  for (int i = 0; i < k; i++)
	  {
		x[i] -= A(i,k) * x[k];
	  }

	  // Multiply by the inverse of the diagonal block.
	  x[k] /= A(k,k);

	  k--;
	}
	else
	{
	  // 2 x 2 diagonal block

	  // Interchange rows K-1 and -IPIV(K).
	  int kp = - pivots[k] - 1;
	  if (kp != k - 1)
	  {
		swap (x[k-1], x[kp]);
	  }

	  // Multiply by inv(U(K)), where U(K) is the transformation
	  // stored in columns K-1 and K of A.
	  for (int i = 0; i < k - 1; i++)
	  {
		x[i] -= A(i,k) * x[k];
	  }
	  for (int i = 0; i < k - 1; i++)
	  {
		x[i] -= A(i,k-1) * x[k-1];
	  }

	  // Multiply by the inverse of the diagonal block.
	  double akm1k = A(k-1,k);
	  double akm1  = A(k-1,k-1) / akm1k;
	  double ak    = A(k,k)     / akm1k;
	  double denom = akm1 * ak - 1;
	  double bkm1  = x[k-1] / akm1k;
	  double bk    = x[k]   / akm1k;
	  x[k-1] = (ak   * bkm1 - bk)   / denom;
	  x[k]   = (akm1 * bk   - bkm1) / denom;

	  k -= 2;
	}
  }

  // Next solve U'*X = B
  // K is the main loop index, increasing from 0 to N-1 in steps of
  // 1 or 2, depending on the size of the diagonal blocks.
  k = 0;
  while (k < n)
  {
	if (pivots[k] > 0)
	{
	  // 1 x 1 diagonal block

	  // Multiply by inv(U'(K)), where U(K) is the transformation
	  // stored in column K of A.
	  for (int i = 0; i < k; i++)
	  {
		x[k] -= A(i,k) * x[i];
	  }

	  // Interchange rows K and IPIV(K)
	  int kp = pivots[k] - 1;
	  if (kp != k)
	  {
		swap (x[k], x[kp]);
	  }

	  k++;
	}
	else
	{
	  // 2 x 2 diagonal block

	  // Multiply by inv(U'(K+1)), where U(K+1) is the transformation
	  // stored in columns K and K+1 of A.
	  for (int i = 0; i < k; i++)
	  {
		x[k] -= A(i,k) * x[i];
	  }
	  for (int i = 0; i < k; i++)
	  {
		x[k+1] -= A(i,k+1) * x[i];
	  }

	  // Interchange rows K and -IPIV(K).
	  int kp = - pivots[k] - 1;
	  if (kp != k)
	  {
		swap (x[k], x[kp]);
	  }

	  k += 2;
	}
  }
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
  Vector<int> ipvt;
  factorize (factoredJJ, ipvt);
  solve (factoredJJ, ipvt, x, Jf);
//cerr << "x1=" << x.frob (1) << " " << fjac.frob (1) << " " << Jf.frob (1) << " " << JJ.frob (1) << " " << factoredJJ.frob (1) << endl;
//cerr << "JJ=" << JJ << endl;
//cerr << "Jf=" << Jf << endl;
//cerr << "x=" << x << endl;
//cerr << endl;

  // Evaluate the function at the origin, and test
  // for acceptance of the gauss-newton direction.
  Vector<double> dx (n);
  for (int j = 0; j < n; j++)
  {
	dx[j] = diag[j] * x[j];
  }
  double dxnorm = dx.frob (2);
  double fp = dxnorm - delta;
//cerr << "fp=" << fp << " " << dxnorm << " " << delta << endl;
//cerr << "x=" << x << endl;
//cerr << "d=" << diag << endl;
//cerr << "dx=" << dx << endl;
//cerr << "dxnorm=" << dxnorm << endl;
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
  solve (factoredJJ, ipvt, wa2, wa1);
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
//cerr << "JJ=" << factoredJJ << endl;
	factorize (factoredJJ, ipvt);
	solve (factoredJJ, ipvt, x, Jf);
//cerr << "x=" << x << endl;
//cerr << endl;

	for (int j = 0; j < n; j++)
	{
	  dx[j] = diag[j] * x[j];
	}
	dxnorm = dx.frob (2);
	double oldFp = fp;
	fp = dxnorm - delta;

//cerr << "par=" << par << " " << parl << " " << paru << " " << fp << " " << delta << endl;
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
	solve (factoredJJ, ipvt, wa2, wa1);
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


// class LevenbergMarquardtBK -------------------------------------------------

LevenbergMarquardtBK::LevenbergMarquardtBK (double toleranceF, double toleranceX, int maxIterations)
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
LevenbergMarquardtBK::search (Searchable & searchable, Vector<double> & point)
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
  int iter = 0;
  while (true)
  {
	iter++;

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
	  Vector<double> xp = point - p;  // p is actually negative
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
	  if (fnorm1 < fnorm * 10)
	  {
		double temp = fnorm1 / fnorm;
		actred = 1 - temp * temp;
	  }
//cerr << "actred=" << actred << " " << fnorm1 << " " << fnorm << endl;

	  // compute the scaled predicted reduction and the scaled directional derivative
	  double temp1 = (fjac * p).frob (2) / fnorm;
	  double temp2 = sqrt (par) * pnorm / fnorm;
	  double prered = temp1 * temp1 + 2 * temp2 * temp2;
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
		double update;
		if (actred >= 0)
		{
		  update = 0.5;
		}
		else
		{
		  update = dirder / (2 * dirder + actred);
		}
		if (fnorm1 >= fnorm * 10  ||  update < 0.1)
		{
		  update = 0.1;
		}
		delta = update * min (delta, pnorm * 10);
		par /= update;
	  }
	  else
	  {
		if (par == 0  ||  ratio >= 0.75)
		{
		  delta = pnorm * 2;
		  par /= 2;
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
	  }
//cerr << "ratio=" << ratio << endl;

	  // tests for convergence
	  if (   fabs (actred) <= toleranceF
		  && prered <= toleranceF
		  && ratio <= 2)
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
		  && ratio <= 2)
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
