#ifndef fl_levenberg_marquardt_tcc
#define fl_levenberg_marquardt_tcc


#include "fl/search.h"
#include "fl/math.h"

#include <float.h>


namespace fl
{
  // class LevenbergMarquardt -------------------------------------------------

  template<class T>
  LevenbergMarquardt<T>::LevenbergMarquardt (T toleranceF, T toleranceX, int maxIterations)
  {
	this->maxIterations = maxIterations;

	if (toleranceF < 0)
	{
	  toleranceF = std::sqrt (epsilon);
	}
	this->toleranceF = toleranceF;

	if (toleranceX < 0)
	{
	  toleranceX = std::sqrt (epsilon);
	}
	this->toleranceX = toleranceX;
  }

  /**
	 A loose paraphrase the MINPACK function lmdif.
  **/
  template<class T>
  void
  LevenbergMarquardt<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	const T toleranceG = 0;

	// Evaluate the function at the starting point and calculate its norm.
	Vector<T> fvec;
	searchable.value (point, fvec);

	int m = fvec.rows ();
	int n = point.rows ();

	Matrix<T> fjac (m, n);
	Vector<T> diag (n);  // scales
	T par = 0;  // levenberg-marquardt parameter
	T fnorm = fvec.frob (2);
	T xnorm;
	T delta;

	// outer loop
	int iter = 0;
	while (true)
	{
	  iter++;

	  // calculate the jacobian matrix
	  searchable.jacobian (point, fjac, &fvec);

	  // compute the qr factorization of the jacobian.
	  Vector<int> ipvt (n);
	  Vector<T> rdiag (n);  // wa1
	  Vector<T> jacobianNorms (n);  // wa2
	  qrfac (fjac, ipvt, rdiag, jacobianNorms);

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
		  T temp = diag[j] * point[j];
		  xnorm += temp * temp;
		}
		xnorm = std::sqrt (xnorm);

		const T factor = 1;
		delta = factor * xnorm;
		if (delta == 0)
		{
		  delta = factor;
		}
	  }

	  // Form (q transpose)*fvec and store the first n components in qtf.
	  // Fix fjac so it contains the diagonal of R rather than tau of Q
	  Vector<T> qtf (n);
	  Vector<T> tempFvec;
	  tempFvec.copyFrom (fvec);
	  for (int j = 0; j < n; j++)
	  {
		T tau = fjac(j,j);
		if (tau != 0)
		{
		  T sum = 0;
		  for (int i = j; i < m; i++)
		  {
			sum += fjac(i,j) * tempFvec[i];
		  }
		  sum /= -tau;
		  for (int i = j; i < m; i++)
		  {
			tempFvec[i] += fjac(i,j) * sum;
		  }
		}
		fjac(j,j) = rdiag[j];  // Replace tau_j with diagonal part of R
		qtf[j] = tempFvec[j];
	  }

	  // compute the norm of the scaled gradient
	  T gnorm = 0;
	  if (fnorm != 0)
	  {
		for (int j = 0; j < n; j++)
		{
		  int l = ipvt[j];
		  if (jacobianNorms[l] != 0)
		  {
			T sum = 0;
			for (int i = 0; i <= j; i++)
			{
			  sum += fjac(i,j) * qtf[i];  // This use of the factored fjac is equivalent to ~fjac * fvec using the original fjac.
			}
			gnorm = std::max (gnorm, std::fabs (sum / (fnorm * jacobianNorms[l])));
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
		diag[j] = std::max (diag[j], jacobianNorms[j]);
	  }

	  // beginning of the inner loop
	  T ratio = 0;
	  while (ratio < 0.0001)
	  {
		// determine the levenberg-marquardt parameter.
		Vector<T> p (n);  // wa1
		lmpar (fjac, ipvt, diag, qtf, delta, par, p);

		// store the direction p and x + p. calculate the norm of p.
		Vector<T> xp = point - p;  // p is actually negative
		T pnorm = 0;
		for (int j = 0; j < n; j++)
		{
		  T temp = diag[j] * p[j];
		  pnorm += temp * temp;
		}
		pnorm = std::sqrt (pnorm);

		// on the first iteration, adjust the initial step bound
		if (iter == 1)
		{
		  delta = std::min (delta, pnorm);
		}

		// evaluate the function at x + p and calculate its norm
		searchable.value (xp, tempFvec);
		T fnorm1 = tempFvec.frob (2);

		// compute the scaled actual reduction
		T actred = -1;
		if (fnorm1 / 10 < fnorm)
		{
		  T temp = fnorm1 / fnorm;
		  actred = 1 - temp * temp;
		}

		// compute the scaled predicted reduction and the scaled directional derivative
		Vector<T> fjacp (n);
		fjacp.clear ();
		for (int j = 0; j < n; j++)
		{
		  T pj = p[ipvt[j]];
		  for (int i = 0; i <= j; i++)
		  {
			fjacp[i] += fjac(i,j) * pj;  // equivalent to fjac * p using the original fjac, since all scale informtion is in the R part of the QR factorizatoion
		  }
		}
		T temp1 = fjacp.frob (2) / fnorm;
		T temp2 = std::sqrt (par) * pnorm / fnorm;
		T prered = temp1 * temp1 + 2 * temp2 * temp2;
		T dirder = -(temp1 * temp1 + temp2 * temp2);

		// compute the ratio of the actual to the predicted reduction
		ratio = 0;
		if (prered != 0)
		{
		  ratio = actred / prered;
		}

		// update the step bound
		if (ratio <= 0.25)
		{
		  T update;
		  if (actred >= 0)
		  {
			update = 0.5;
		  }
		  else
		  {
			update = dirder / (2 * dirder + actred);
		  }
		  if (fnorm1 / 10 >= fnorm  ||  update < 0.1)
		  {
			update = 0.1;
		  }
		  delta = update * std::min (delta, pnorm * 10);
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
			T temp = diag[j] * point[j];
			xnorm += temp * temp;
		  }
		  xnorm = std::sqrt (xnorm);

		  fnorm = fnorm1;
		}

		// tests for convergence
		if (   std::fabs (actred) <= toleranceF
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
		if (   std::fabs (actred) <= epsilon
			&& prered <= epsilon
			&& ratio <= 2)
		{
		  throw (int) 6;
		}
		if (delta <= epsilon * xnorm)
		{
		  throw (int) 7;
		}
		if (gnorm <= epsilon)
		{
		  throw (int) 8;
		}
	  }
	}
  }

  template<class T>
  void
  LevenbergMarquardt<T>::qrfac (Matrix<T> & a, Vector<int> & ipvt, Vector<T> & rdiag, Vector<T> & acnorm)
  {
	int m = a.rows ();
	int n = a.columns ();
	Vector<T> wa (n);

	// Compute the initial column norms and initialize several arrays.
	for(int j = 0; j < n; j++)
	{
	  wa[j] = rdiag[j] = acnorm[j] = a.column (j).frob (2);
	  ipvt[j] = j;
	}

	// Reduce a to r with householder transformations.
	int minmn = std::min (m, n);
	for (int j = 0; j < minmn; j++ )
	{
	  // Bring the column of largest norm into the pivot position.
	  int kmax = j;
	  for(int k = j + 1; k < n; k++)
	  {
		if (rdiag[k] > rdiag[kmax])
		{
		  kmax = k;
		}
	  }

	  if (kmax != j)
	  {
		Vector<T> temp = a.column (j);
		a.column (j) = a.column (kmax);
		a.column (kmax) = temp;

		rdiag[kmax] = rdiag[j];
		wa[kmax] = wa[j];

		std::swap (ipvt[j], ipvt[kmax]);
	  }

	  // Compute the householder transformation to reduce the
	  // j-th column of a to a multiple of the j-th unit vector.
	  MatrixRegion<T> jthColumn (a, j, j, m-1, j);  // Actually, lower portion of column
	  T ajnorm = jthColumn.frob (2);
	  if (ajnorm != 0)
	  {
		if (a(j,j) < 0)
		{
		  ajnorm = -ajnorm;
		}

		jthColumn /= ajnorm;
		a(j,j) += 1;

		// Apply the transformation to the remaining columns and update the norms.
		//jp1 = j + 1;
		for(int k = j + 1; k < n; k++)
		{
		  MatrixRegion<T> kthColumn (a, j, k, m-1, k);
		  kthColumn -= jthColumn * (jthColumn.dot (kthColumn) / a(j,j));

		  if(rdiag[k] != 0)
		  {
			T temp = a(j,k) / rdiag[k];
			rdiag[k] *= std::sqrt (std::max ((T) 0, 1 - temp * temp));
			temp = rdiag[k] / wa[k];
			if (0.05 * temp * temp <= epsilon)
			{
			  rdiag[k] = MatrixRegion<T> (a, j+1, k, m-1, k).frob (2);
			  wa[k] = rdiag[k];
			}
		  }
		}
	  }

	  rdiag[j] = -ajnorm;
	}
  }

  template<class T>
  void
  LevenbergMarquardt<T>::qrsolv (Matrix<T> & r, const Vector<int> & ipvt, const Vector<T> & diag, const Vector<T> & qtb, Vector<T> & x, Vector<T> & sdiag)
  {
	int n = r.columns ();
	Vector<T> wa (n);

	// Copy r and (q transpose)*b to preserve input and initialize s.
	// In particular, save the diagonal elements of r in x.
	for (int j = 0; j < n; j++)
	{
	  for (int i = j + 1; i < n; i++)
	  {
		r(i,j) = r(j,i);
	  }
	  x[j] = r(j,j);
	  wa[j] = qtb[j];
	}

	// Eliminate the diagonal matrix d using a givens rotation.
	for (int j = 0; j < n; j++)
	{
	  // Prepare the row of d to be eliminated, locating the
	  // diagonal element using p from the qr factorization.
	  int l = ipvt[j];
	  if (diag[l] != 0)
	  {
		sdiag[j] = diag[l];
		for (int k = j + 1; k < n; k++)
		{
		  sdiag[k] = 0;
		}

		// The transformations to eliminate the row of d
		// modify only a single element of (q transpose)*b
		// beyond the first n, which is initially zero.
		T qtbpj = 0;
		for (int k = j; k < n; k++)
		{
		  // Determine a givens rotation which eliminates the
		  // appropriate element in the current row of d.
		  if (sdiag[k] == 0)
		  {
			continue;
		  }
		  T sin;
		  T cos;
		  if (std::fabs (r(k,k)) < std::fabs (sdiag[k]))
		  {
			T cotan = r(k,k) / sdiag[k];
			sin = 0.5 / std::sqrt (0.25 + 0.25 * cotan * cotan);
			cos = sin * cotan;
		  }
		  else
		  {
			T tan = sdiag[k] / r(k,k);
			cos = 0.5 / std::sqrt (0.25 + 0.25 * tan * tan);
			sin = cos * tan;
		  }

		  // Compute the modified diagonal element of r and
		  // the modified element of ((q transpose)*b,0).
		  r(k,k) = cos * r(k,k) + sin * sdiag[k];
		  T temp = cos * wa[k] + sin * qtbpj;
		  qtbpj = -sin * wa[k] + cos * qtbpj;
		  wa[k] = temp;

		  // Accumulate the tranformation in the row of s.
		  for (int i = k + 1; i < n; i++)
		  {
			T temp = cos * r(i,k) + sin * sdiag[i];
			sdiag[i] = -sin * r(i,k) + cos * sdiag[i];
			r(i,k) = temp;
		  }
		}
	  }

	  // Store the diagonal element of s and restore
	  // the corresponding diagonal element of r.
	  sdiag[j] = r(j,j);
	  r(j,j) = x[j];
	}

	// Solve the triangular system for z. if the system is
	// singular, then obtain a least squares solution.
	int nsing = n;
	for (int j = 0; j < n; j++)
	{
	  if (sdiag[j] == 0  &&  nsing == n)
	  {
		nsing = j;
	  }
	  if (nsing < n)
	  {
		wa[j] = 0;
	  }
	}

	for (int k = 0; k < nsing; k++)
	{
	  int j = (nsing - 1) - k;
	  T sum = 0;
	  for (int i = j + 1; i < nsing; i++)
	  {
		sum += r(i,j) * wa[i];
	  }
	  wa[j] = (wa[j] - sum) / sdiag[j];
	}

	// Permute the components of z back to components of x.
	for (int j = 0; j < n; j++)
	{
	  x[ipvt[j]] = wa[j];
	}
  }

  /**
	 lmpar algorithm:

	 A constrained lls problem:
	   solve (~JJ + pDD)x = ~Jf
	   such that |Dx| is pretty close to delta

	 Start with p = 0 and determine x
	   Solve for x in ~JJx = ~Jf
	   Early out if |Dx| is close to delta
	 Determine min and max values for p
	   J = QR  (so ~JJ = ~RR)
	   solve for b in ~Rb = DDx / |Dx|
	   min = (|Dx| - delta) / (delta * |b|^2)
	   max = |!D~Jf| / delta
	 Initialize p
	   make sure it is in bounds
	   if p is zero, p = |!D~Jf| / |Dx|
	 Iterate
	   solve for x in (~JJ + pDD)x = ~Jf
	   end if |Dx| is close to delta
	     or too many iterations
		 or |Dx| is becoming smaller than delta when min == 0
	   (~JJ + pDD) = QR
	   solve for b in ~Rb = DDx / |Dx|
	   p += (|Dx| - delta) / (delta * |b|^2)
  **/
  template<class T>
  void
  LevenbergMarquardt<T>::lmpar (Matrix<T> & r, const Vector<int> & ipvt, const Vector<T> & diag, const Vector<T> & qtb, T delta, T & par, Vector<T> & x)
  {
	int n = r.columns ();

	Vector<T> sdiag (n);
	Vector<T> wa1 (n);
	Vector<T> dx (n);

	// Compute and store in x the gauss-newton direction. If the
	// jacobian is rank-deficient, obtain a least squares solution.
	int nsing = n;
	for (int j = 0; j < n; j++)
	{
	  if (r(j,j) == 0  &&  nsing == n)
	  {
		nsing = j;
	  }
	  if (nsing < n)
	  {
		wa1[j] = 0;
	  }
	  else
	  {
		wa1[j] = qtb[j];
	  }
	}
	// solve for x by back-substitution in rx=qtb (which comes from qrx=b, which
	// comes from ax=b, where a=fjac)
	for (int k = 0; k < nsing; k++)
	{
	  int j = (nsing - 1) - k;
	  T temp = wa1[j] /= r(j,j);
	  for (int i = 0; i < j; i++)
	  {
		wa1[i] -= r(i,j) * temp;
	  }
	}
	for (int j = 0; j < n; j++)
	{
	  x[ipvt[j]] = wa1[j];
	}

	// Initialize the iteration counter.
	// Evaluate the function at the origin, and test
	// for acceptance of the gauss-newton direction.
	for (int j = 0; j < n; j++)
	{
	  dx[j] = diag[j] * x[j];
	}
	T dxnorm = dx.frob (2);
	T fp = dxnorm - delta;
	if (fp <= 0.1 * delta)
	{
	  par = 0;
	  return;
	}

	// If the jacobian is not rank deficient, the newton
	// step provides a lower bound, parl, for the zero of
	// the function. otherwise set this bound to zero.
	T parl = 0;
	if (nsing == n)
	{
	  for (int j = 0; j < n; j++)
	  {
		int l = ipvt[j];
		wa1[j] = diag[l] * (dx[l] / dxnorm);
	  }
	  // solve by back-substitution for b in rtb=x (where "x" = d*d*x and x is
	  // normalized).  note that rt is lower triangular, and that back-sub
	  // starts at top row rather than bottom
	  for (int j = 0; j < n; j++)
	  {
		T sum = 0;
		for (int i = 0; i < j; i++)
		{
		  sum += r(i,j) * wa1[i];
		}
		wa1[j] = (wa1[j] - sum) / r(j,j);
	  }

	  T temp = wa1.frob (2);
	  parl = ((fp / delta) / temp) / temp;
	}

	// Calculate an upper bound, paru, for the zero of the function.
	for (int j = 0; j < n; j++)
	{
	  T sum = 0;
	  for (int i = 0; i <= j; i++)
	  {
		sum += r(i,j) * qtb[i];  // equivalent to ~fjac * fvec before factoriztion
	  }
	  wa1[j] = sum / diag[ipvt[j]];
	}

	T gnorm = wa1.frob (2);
	T paru = gnorm / delta;
	if (paru == 0)
	{
	  paru = minimum / std::min (delta, (T) 0.1);
	}

	// If the input par lies outside of the interval (parl,paru),
	// set par to the closer endpoint.
	par = std::max (par, parl);
	par = std::min (par, paru);
	if (par == 0)
	{
	  par = gnorm / dxnorm;
	}

	int iter = 0;
	while (true)
	{
	  iter++;

	  // Evaluate the function at the current value of par.
	  if (par == (T) 0)
	  {
		par = std::max (minimum, (T) 0.001 * paru);
	  }
	  T temp = std::sqrt (par);
	  for (int j = 0; j < n; j++)
	  {
		wa1[j] = temp * diag[j];
	  }

	  qrsolv (r, ipvt, wa1, qtb, x, sdiag);

	  for (int j = 0; j < n; j++)
	  {
		dx[j] = diag[j] * x[j];
	  }

	  dxnorm = dx.frob (2);
	  temp = fp;
	  fp = dxnorm - delta;

	  // If the function is small enough, accept the current value
	  // of par.  Also test for the exceptional cases where parl
	  // is zero or the number of iterations has reached 10.
	  if (   std::fabs (fp) <= 0.1 * delta
		  || (parl == 0  &&  fp <= temp  &&  temp < 0)
		  || iter >= 10)
	  {
		return;
	  }

	  // Compute the newton correction.
	  for (int j = 0; j < n; j++)
	  {
		int l = ipvt[j];
		wa1[j] = diag[l] * (dx[l] / dxnorm);
	  }
	  for (int j = 0; j < n; j++)
	  {
		temp = wa1[j] /= sdiag[j];
		for (int i = j + 1; i < n; i++)
		{
		  wa1[i] -= r(i,j) * temp;
		}
	  }

	  temp = wa1.frob (2);
	  T parc = ((fp / delta) / temp) / temp;

	  // Depending on the sign of the function, update parl or paru.
	  if (fp > 0)
	  {
		parl = std::max (parl, par);
	  }
	  if (fp < 0)
	  {
		paru = std::min (paru, par);
	  }
	  // Compute an improved estimate for par.
	  par = std::max (parl, par + parc);
	}
  }
}


#endif
