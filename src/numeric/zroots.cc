/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/zroots.h"


#define EPSS 1.0e-12
#define MR 8
#define MT 10
#define MAXIT (MT*MR)


using namespace fl;
using namespace std;


int
fl::laguer (const Vector<complex<double> > & a, complex<double> & x)
{
  int m = a.rows () - 1;

  int iter, j;
  double abx, abp, abm, err;
  complex<double> dx, x1, b, d, f, g, h, sq, gp, gm, g2;
  static double frac[MR+1] = {0.0, 0.5, 0.25, 0.75, 0.13, 0.38, 0.62, 0.88, 1.0};
  complex<double> tx = x; 

  for (iter = 1; iter <= MAXIT; iter++)
  {
    b = a[m];
    err = abs (b);
    d = f = 0;
    abx = abs (tx);
    for (j = m - 1; j >= 0; j--)
	{
      f *= tx; 
	  f += d;
	  d *= tx;
	  d += b;
	  b *= tx;
	  b += a[j];

      err *= abx;
	  err += abs (b);
    }
    err *= EPSS;
    
    if (abs (b) <= err)
	{
	  x = tx;
	  return iter;
	}
    g = d/b;
    g2 = g*g;
    h = g2 - f/b * 2.0;
    sq = sqrt (((double) m * h - g2) * (double) (m - 1));
    gp = g + sq;
    gm = g - sq;
    abp = abs (gp);
    abm = abs (gm);
    if (abp < abm)
	{
	  gp=gm;
	}
    
    dx = ((abp > 0.0) || (abm > 0))
	  ? (double) m / gp
	  : polar (exp (log (1 + abx)), (double) iter);

    x1 = tx - dx;
    if (tx == x1)
	{
	  x = tx;
	  return iter;
	}
    if (iter % MT)
	{
	  tx = x1;
	}
    else
	{
	  tx = tx - frac[iter / MT] * dx;
	}
  }

  throw "too many iterations in laguer";
}


#define EPS 2.0e-8
#define MAXM  30          /* Maximum degree */

void
fl::zroots(const Vector<complex<double> > & a, Vector<complex<double> > & roots, bool polish, bool sortroots)
{
  int m = a.rows () - 1;

  Vector<complex<double> > ad;
  ad.copyFrom (a);

  roots.resize (m);  // number of roots is exactly same as degree

  for (int j = m; j >= 1; j--)
  {
	ad.rows_ = j + 1;  //  gross hack
    complex<double> x = 0;
    laguer (ad, x);
    if (fabs (imag (x)) <= 2.0 * EPS * fabs (real (x)))
	{
	  x = real (x);
	}
    roots[j - 1] = x;

    complex<double> b = ad[j];
    for(int jj = j - 1; jj >= 0; jj--)
	{
      complex<double> c = ad[jj];
      ad[jj] = b;
      b = x * b + c;
    }
  }

  if (polish)
  {
    for (int j = 0; j < m; j++)
	{
      laguer (a, roots[j]);
	}
  }

  if (sortroots)
  {
    for (int j = 1; j < m; j++)
	{
      complex<double> x = roots[j];
	  int i;
      for (i = j - 1; i >= 0; i--)
	  {
		if (real (roots[i]) <= real(x))
		{
		  break;
		}
		roots[i+1] = roots[i];
      }
      roots[i+1] = x;
    }
  }
}
