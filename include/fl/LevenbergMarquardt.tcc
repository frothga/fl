/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_levenberg_marquardt_tcc
#define fl_levenberg_marquardt_tcc


#include "fl/search.h"
#include "fl/math.h"

#include <float.h>
#include <limits>


namespace fl
{
  // class LevenbergMarquardt -------------------------------------------------

  template<class T>
  LevenbergMarquardt<T>::LevenbergMarquardt (T toleranceF, T toleranceX, int maxIterations)
  {
	this->maxIterations = maxIterations;

	if (toleranceF < (T) 0) toleranceF = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceF = toleranceF;

	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceX = toleranceX;
  }

  /**
	 A loose paraphrase the MINPACK function lmdif.
  **/
  template<class T>
  void
  LevenbergMarquardt<T>::search (Searchable<T> & searchable, Vector<T> & x)
  {
	const T toleranceG = 0;
	const T epsilon = std::numeric_limits<T>::epsilon ();

	// Evaluate the function at the starting x and calculate its norm.
	Vector<T> y;
	searchable.value (x, y);

	int m = y.rows ();
	int n = x.rows ();

	Matrix<T> J (m, n);
	Vector<T> scales (n);
	T par = 0;  // levenberg-marquardt parameter
	T ynorm = y.norm (2);
	T xnorm;
	T delta;

	// outer loop
	for (int iteration = 0; iteration < maxIterations; iteration++)
	{
	  // calculate the jacobian matrix
	  searchable.jacobian (x, J, &y);

	  // compute the qr factorization of the jacobian.
	  Vector<int> pivots (n);
	  Vector<T> rdiag (n);  // wa1
	  Vector<T> jacobianNorms (n);  // wa2
	  qrfac (J, pivots, rdiag, jacobianNorms);

	  // On the first iteration ...
	  if (iteration == 0)
	  {
		// Scale according to the norms of the columns of the initial jacobian.
		for (int j = 0; j < n; j++)
		{
		  scales[j] = jacobianNorms[j];
		  if (scales[j] == 0)
		  {
			scales[j] = 1;
		  }
		}

		// Calculate the norm of the scaled x and initialize the step bound delta.
		xnorm = (x & scales).norm (2);
		delta = (xnorm * (T) 1) == (T) 0 ? (T) 1 : xnorm;  // Does the multiplication by 1 here make a difference?  This seems to be a test of floating-point representation.
	  }

	  // Form (q transpose)*y and store the first n components in qtf.
	  // Fix J so it contains the diagonal of R rather than tau of Q
	  Vector<T> qtf (n);
	  Vector<T> tempY;
	  tempY.copyFrom (y);
	  for (int j = 0; j < n; j++)
	  {
		T tau = J(j,j);
		if (tau != 0)
		{
		  T sum = 0;
		  for (int i = j; i < m; i++)
		  {
			sum += J(i,j) * tempY[i];
		  }
		  sum /= -tau;
		  for (int i = j; i < m; i++)
		  {
			tempY[i] += J(i,j) * sum;
		  }
		}
		J(j,j) = rdiag[j];  // Replace tau_j with diagonal part of R
		qtf[j] = tempY[j];
	  }

	  // compute the norm of the scaled gradient
	  T gnorm = 0;
	  if (ynorm != 0)
	  {
		for (int j = 0; j < n; j++)
		{
		  int l = pivots[j];
		  if (jacobianNorms[l] != 0)
		  {
			T sum = 0;
			for (int i = 0; i <= j; i++)
			{
			  sum += J(i,j) * qtf[i];  // This use of the factored J is equivalent to ~J * y using the original J.  (That is, ~R * ~Q * y, where J = QR)
			}
			gnorm = std::max (gnorm, std::fabs (sum / (ynorm * jacobianNorms[l])));  // infinity norm of g = ~J * y / |y|.
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
		scales[j] = std::max (scales[j], jacobianNorms[j]);
	  }

	  // beginning of the inner loop
	  T ratio = 0;
	  while (ratio < 0.0001)
	  {
		// determine the levenberg-marquardt parameter.
		Vector<T> p (n);  // wa1
		lmpar (J, pivots, scales, qtf, delta, par, p);

		// store the direction p and x + p. calculate the norm of p.
		Vector<T> xp = x - p;  // p is actually negative
		T pnorm = (p & scales).norm (2);

		// on the first iteration, adjust the initial step bound
		if (iteration == 0)
		{
		  delta = std::min (delta, pnorm);
		}

		// evaluate the function at x + p and calculate its norm
		searchable.value (xp, tempY);
		T ynorm1 = tempY.norm (2);

		// compute the scaled actual reduction
		T reductionActual = -1;
		if (ynorm1 / 10 < ynorm)
		{
		  T temp = ynorm1 / ynorm;
		  reductionActual = 1 - temp * temp;
		}

		// compute the scaled predicted reduction and the scaled directional derivative
		Vector<T> Jp (n);
		Jp.clear ();
		for (int j = 0; j < n; j++)
		{
		  T pj = p[pivots[j]];
		  for (int i = 0; i <= j; i++)
		  {
			Jp[i] += J(i,j) * pj;  // equivalent to J * p using the original J, since all scale informtion is in the R part of the QR factorizatoion
		  }
		}
		T temp1 = Jp.norm (2) / ynorm;
		T temp2 = std::sqrt (par) * pnorm / ynorm;
		T reductionPredicted = temp1 * temp1 + 2 * temp2 * temp2;
		T dirder = -(temp1 * temp1 + temp2 * temp2);

		// compute the ratio of the actual to the predicted reduction
		ratio = 0;
		if (reductionPredicted != 0)
		{
		  ratio = reductionActual / reductionPredicted;
		}

		// update the step bound
		if (ratio <= 0.25)
		{
		  T update;
		  if (reductionActual >= 0)
		  {
			update = 0.5;
		  }
		  else
		  {
			update = dirder / (2 * dirder + reductionActual);
		  }
		  if (ynorm1 / 10 >= ynorm  ||  update < 0.1)
		  {
			update = (T) 0.1;
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

		if (ratio >= 0.0001)  // successful iteration.
		{
		  // update x, y, and their norms
		  x     = xp;
		  y     = tempY;
		  xnorm = (x & scales).norm (2);
		  ynorm = ynorm1;
		}

		// tests for convergence
		if (   std::fabs (reductionActual) <= toleranceF
			&& reductionPredicted <= toleranceF
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
		if (   std::fabs (reductionActual) <= epsilon
			&& reductionPredicted <= epsilon
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

	throw (int) 5;  // exceeded maximum iterations
  }

  template<class T>
  void
  LevenbergMarquardt<T>::qrfac (Matrix<T> & a, Vector<int> & pivots, Vector<T> & rdiag, Vector<T> & acnorm)
  {
	const T epsilon = std::numeric_limits<T>::epsilon ();
	const int m = a.rows ();
	const int n = a.columns ();
	Vector<T> wa (n);

	// Compute the initial column norms and initialize several arrays.
	for(int j = 0; j < n; j++)
	{
	  wa[j] = rdiag[j] = acnorm[j] = a.column (j).norm (2);
	  pivots[j] = j;
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

		std::swap (pivots[j], pivots[kmax]);
	  }

	  // Compute the householder transformation to reduce the
	  // j-th column of a to a multiple of the j-th unit vector.
	  MatrixRegion<T> jthColumn (a, j, j, m-1, j);  // Actually, lower portion of column
	  T ajnorm = jthColumn.norm (2);
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
			  rdiag[k] = MatrixRegion<T> (a, j+1, k, m-1, k).norm (2);
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
  LevenbergMarquardt<T>::qrsolv (Matrix<T> & r, const Vector<int> & pivots, const Vector<T> & scales, const Vector<T> & qtb, Vector<T> & x, Vector<T> & sdiag)
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
	  int l = pivots[j];
	  if (scales[l] != 0)
	  {
		sdiag[j] = scales[l];
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
	  x[pivots[j]] = wa[j];
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
  LevenbergMarquardt<T>::lmpar (Matrix<T> & r, const Vector<int> & pivots, const Vector<T> & scales, const Vector<T> & qtb, T delta, T & par, Vector<T> & x)
  {
	const T minimum = std::numeric_limits<T>::min ();
	const int n = r.columns ();

	Vector<T> sdiag (n);
	Vector<T> wa1 (n);

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
	// comes from ax=b, where a=J)
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
	  x[pivots[j]] = wa1[j];
	}

	// Initialize the iteration counter.
	// Evaluate the function at the origin, and test
	// for acceptance of the gauss-newton direction.
	Vector<T> dx = x & scales;
	T dxnorm = dx.norm (2);
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
		int l = pivots[j];
		wa1[j] = scales[l] * (dx[l] / dxnorm);
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

	  T temp = wa1.norm (2);
	  parl = ((fp / delta) / temp) / temp;
	}

	// Calculate an upper bound, paru, for the zero of the function.
	for (int j = 0; j < n; j++)
	{
	  T sum = 0;
	  for (int i = 0; i <= j; i++)
	  {
		sum += r(i,j) * qtb[i];  // equivalent to ~J * y before factoriztion
	  }
	  wa1[j] = sum / scales[pivots[j]];
	}

	T gnorm = wa1.norm (2);
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
		wa1[j] = temp * scales[j];
	  }

	  qrsolv (r, pivots, wa1, qtb, x, sdiag);

	  dx = x & scales;
	  dxnorm = dx.norm (2);
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
		int l = pivots[j];
		wa1[j] = scales[l] * (dx[l] / dxnorm);
	  }
	  for (int j = 0; j < n; j++)
	  {
		temp = wa1[j] /= sdiag[j];
		for (int i = j + 1; i < n; i++)
		{
		  wa1[i] -= r(i,j) * temp;
		}
	  }

	  temp = wa1.norm (2);
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
