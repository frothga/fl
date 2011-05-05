/*
Author: Fred Rothganger

Copyright 2005-2008, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/

/**
   Functions to convert between various parameterizations of orientation.

   Most functions are of the form A2B(), where A and B are abbreviations for
   different parameterizations.  Abbreviations:
     R -- 3x3 rotation matrix
	 V -- 4-vector containing a quaternion with the real part in element 0
	 D -- Rodriguez vector
	 M -- 4x4 matrix that implements quaternion product:  V = M(V1)V2  <-->  q = q1 * q2
	 XYZ -- Euler angles.  Exact sequence of axis names gives the convention.  Leftmost letter
	        specifies first axis to rotate around.  The basis vectors x, y and z around which the
			rotations occur remain constant throughout all three rotations.  However, Euler angles
			often refer to a frame that changes as rotations are applied.  To adjust for this,
			reverse the sequence of axes (but do not change the sign of the angles.)

   @todo These functions should be rewritten to return MatrixResult.
   @todo Eventually, it may make sense to encapsulate various orientation
   representations into classes.
**/

#ifndef rotation_h
#define rotation_h


#include <fl/matrix.h>
#include <fl/lapack.h>


inline fl::Vector<double>
conjugate (const fl::Vector<double> & V)
{
  fl::Vector<double> result (4);
  result[0] =  V[0];
  result[1] = -V[1];
  result[2] = -V[2];
  result[3] = -V[3];
  return result;
}

inline fl::Vector<double>
multiply (const fl::Vector<double> & a, const fl::Vector<double> & b)
{
  const double & a0 = a[0];
  const double & a1 = a[1];
  const double & a2 = a[2];
  const double & a3 = a[3];

  const double & b0 = b[0];
  const double & b1 = b[1];
  const double & b2 = b[2];
  const double & b3 = b[3];

  fl::Vector<double> result (4);
  result[0] = a0 * b0 - a1 * b1 - a2 * b2 - a3 * b3;
  result[1] = a0 * b1 + a1 * b0 + a2 * b3 - a3 * b2;
  result[2] = a0 * b2 - a1 * b3 + a2 * b0 + a3 * b1;
  result[3] = a0 * b3 + a1 * b2 - a2 * b1 + a3 * b0;
  return result;
}

inline fl::Matrix<double>
V2M (const fl::Vector<double> & V)
{
  fl::Matrix<double> M (4, 4);
  M(0,0) =  V[0];
  M(1,0) =  V[1];
  M(2,0) =  V[2];
  M(3,0) =  V[3];
  M(0,1) = -V[1];
  M(1,1) =  V[0];
  M(2,1) =  V[3];
  M(3,1) = -V[2];
  M(0,2) = -V[2];
  M(1,2) = -V[3];
  M(2,2) =  V[0];
  M(3,2) =  V[1];
  M(0,3) = -V[3];
  M(1,3) =  V[2];
  M(2,3) = -V[1];
  M(3,3) =  V[0];
  return M;
}

/**
   Any time you form a Jacobian for a function with a quaternion as input,
   it is necessary to postmultiply it by the implied normalization of the
   quaternion.
**/
inline fl::Matrix<double>
jacobianNormalizeV (const fl::Vector<double> & V)
{
  fl::Matrix<double> N (4, 4);

  double r = V[0];
  double i = V[1];
  double j = V[2];
  double k = V[3];

  double rr = r * r;
  double ri = r * i;
  double rj = r * j;
  double rk = r * k;
  double ii = i * i;
  double ij = i * j;
  double ik = i * k;
  double jj = j * j;
  double jk = j * k;
  double kk = k * k;

  N(0,0) = 1.0 - rr;
  N(1,0) = -ri;
  N(2,0) = -rj;
  N(3,0) = -rk;
  N(0,1) = -ri;
  N(1,1) = 1.0 - ii;
  N(2,1) = -ij;
  N(3,1) = -ik;
  N(0,2) = -rj;
  N(1,2) = -ij;
  N(2,2) = 1.0 - jj;
  N(3,2) = -jk;
  N(0,3) = -rk;
  N(1,3) = -ik;
  N(2,3) = -jk;
  N(3,3) = 1.0 - kk;

  return N;
}

/**
   Returns the rate of change in the elements of M(normalize(V)) with
   respect to V.  M is vectorized in column major form.
**/
inline fl::Matrix<double>
jacobianV2M (const fl::Vector<double> & V)
{
  fl::Matrix<double> dM (16, 4);
  dM.clear ();

  dM(0, 0) =  1;
  dM(5, 0) =  1;
  dM(10,0) =  1;
  dM(15,0) =  1;
  dM(1, 1) =  1;
  dM(4, 1) = -1;
  dM(11,1) =  1;
  dM(14,1) = -1;
  dM(2, 2) =  1;
  dM(7, 2) = -1;
  dM(8, 2) = -1;
  dM(13,2) =  1;
  dM(3, 3) =  1;
  dM(6, 3) =  1;
  dM(9, 3) = -1;
  dM(12,3) = -1;

  return dM * jacobianNormalizeV (V);
}

/**
   Returns the rate of change in the result of V2M(normalize(V1))*V2 with
   respect to V1.  This is equivalent to finding the Jacobian of the
   quaternion product V1*V2 with respect to V1 (while V2 is constant).  This
   is a convenience function to avoid a costly and complex premultiplication
   with the result of jacobianV2M(V).
**/
inline fl::Matrix<double>
jacobianV2M (const fl::Vector<double> & V1, const fl::Vector<double> & V2)
{
  fl::Matrix<double> dM (4, 4);
  dM.clear ();

  dM(0,0) =  V2[0];
  dM(1,0) =  V2[1];
  dM(2,0) =  V2[2];
  dM(3,0) =  V2[3];
  dM(0,1) = -V2[1];
  dM(1,1) =  V2[0];
  dM(2,1) = -V2[3];
  dM(3,1) =  V2[2];
  dM(0,2) = -V2[2];
  dM(1,2) =  V2[3];
  dM(2,2) =  V2[0];
  dM(3,2) = -V2[1];
  dM(0,3) = -V2[3];
  dM(1,3) = -V2[2];
  dM(2,3) =  V2[1];
  dM(3,3) =  V2[0];

  return dM * jacobianNormalizeV (V1);
}

inline fl::Matrix<double>
V2R (const fl::Vector<double> & V)
{
  double ri = V[0] * V[1];
  double rj = V[0] * V[2];
  double rk = V[0] * V[3];
  double ii = V[1] * V[1];
  double ij = V[1] * V[2];
  double ik = V[1] * V[3];
  double jj = V[2] * V[2];
  double jk = V[2] * V[3];
  double kk = V[3] * V[3];

  fl::Matrix<double> R (3, 3);
  R(0,0) = 1.0 - 2.0 * (jj + kk);
  R(1,0) = 2.0 * (ij + rk);
  R(2,0) = 2.0 * (ik - rj);
  R(0,1) = 2.0 * (ij - rk);
  R(1,1) = 1.0 - 2.0 * (ii + kk);
  R(2,1) = 2.0 * (jk + ri);
  R(0,2) = 2.0 * (ik + rj);
  R(1,2) = 2.0 * (jk - ri);
  R(2,2) = 1.0 - 2.0 * (ii + jj);
  return R;
}

/**
	Returns the Jacobian of the function V2R(normalize(V)).  Assumes that
	V always has unit length.  R is vectorized in column-major order.
**/
inline fl::Matrix<double>
jacobianV2R (const fl::Vector<double> & V)
{
  fl::Matrix<double> J (9, 4);

  const double r = V[0];
  const double i = V[1];
  const double j = V[2];
  const double k = V[3];

  J(0,0) = 0;
  J(1,0) =  2.0 * k;
  J(2,0) = -2.0 * j;
  J(3,0) = -2.0 * k;
  J(4,0) = 0;
  J(5,0) =  2.0 * i;
  J(6,0) =  2.0 * j;
  J(7,0) = -2.0 * i;
  J(8,0) = 0;

  J(0,1) = 0;
  J(1,1) =  2.0 * j;
  J(2,1) =  2.0 * k;
  J(3,1) =  2.0 * j;
  J(4,1) = -4.0 * i;
  J(5,1) =  2.0 * r;
  J(6,1) =  2.0 * k;
  J(7,1) = -2.0 * r;
  J(8,1) = -4.0 * i;

  J(0,2) = -4.0 * j;
  J(1,2) =  2.0 * i;
  J(2,2) = -2.0 * r;
  J(3,2) =  2.0 * i;
  J(4,2) = 0;
  J(5,2) =  2.0 * k;
  J(6,2) =  2.0 * r;
  J(7,2) =  2.0 * k;
  J(8,2) = -4.0 * j;

  J(0,3) = -4.0 * k;
  J(1,3) =  2.0 * r;
  J(2,3) =  2.0 * i;
  J(3,3) = -2.0 * r;
  J(4,3) = -4.0 * k;
  J(5,3) =  2.0 * j;
  J(6,3) =  2.0 * i;
  J(7,3) =  2.0 * j;
  J(8,3) = 0;

  return J * jacobianNormalizeV (V);
}

/**
   Returns the Jacobian of the function V2R(normalize(V))*a, where "a" is a
   constant 3-vector to be rotated by the output of V2R().  This is a
   convenience function to avoid a costly and complex premultiplication with
   the result of jacobianV2R(V).
**/
inline fl::Matrix<double>
jacobianV2R (const fl::Vector<double> & V, const fl::Vector<double> & a)
{
  fl::Matrix<double> J (3, 4);

  const double r = V[0];
  const double i = V[1];
  const double j = V[2];
  const double k = V[3];

  const double a0 = a[0];
  const double a1 = a[1];
  const double a2 = a[2];

  J(0,0) =               - 2.0 * k * a1 + 2.0 * j * a2;
  J(1,0) =  2.0 * k * a0                - 2.0 * i * a2;
  J(2,0) = -2.0 * j * a0 + 2.0 * i * a1;

  J(0,1) =                 2.0 * j * a1 + 2.0 * k * a2;
  J(1,1) =  2.0 * j * a0 - 4.0 * i * a1 - 2.0 * r * a2;
  J(2,1) =  2.0 * k * a0 + 2.0 * r * a1 - 4.0 * i * a2;

  J(0,2) = -4.0 * j * a0 + 2.0 * i * a1 + 2.0 * r * a2;
  J(1,2) =  2.0 * i * a0                + 2.0 * k * a2;
  J(2,2) = -2.0 * r * a0 + 2.0 * k * a1 - 4.0 * j * a2;

  J(0,3) = -4.0 * k * a0 - 2.0 * r * a1 + 2.0 * i * a2;
  J(1,3) =  2.0 * r * a0 - 4.0 * k * a1 + 2.0 * j * a2;
  J(2,3) =  2.0 * i * a0 + 2.0 * j * a1;

  return J * jacobianNormalizeV (V);
}

inline fl::Vector<double>
V2D (const fl::Matrix<double> & V)
{
  const double & r = V[0];
  const double & i = V[1];
  const double & j = V[2];
  const double & k = V[3];

  double n = sqrt (i * i + j * j + k * k);
  double theta = 2 * atan (n / r);
  double tn = theta / n;

  fl::Vector<double> D (3);
  D[0] = i * tn;
  D[1] = j * tn;
  D[2] = k * tn;
  return D;
}

inline fl::Matrix<double>
jacobianV2D (const fl::Vector<double> & V)
{
  const double & r = V[0];
  const double & i = V[1];
  const double & j = V[2];
  const double & k = V[3];

  double n     = i * i + j * j + k * k;
  double n12   = sqrt (n);  // n^{1/2}
  double n2    = n * n;     // n^2
  double n32   = n * n12;   // n^{3/2}
  double nr    = n + r * r;
  double atn   = atan (n12 / r);
  double n2rnr = n2 / r + n * r;
  double atn12 = atn / n12;
  double atn32 = atn / n32;

  fl::Matrix<double> J (3, 4);

  J(0,0) = -2.0 * i / nr;
  J(1,0) = -2.0 * j / nr;
  J(2,0) = -2.0 * k / nr;

  J(0,1) = 2.0 * (atn12 + i * i / n2rnr - i * i * atn32);
  J(1,1) = 2.0 * (        j * i / n2rnr - j * i * atn32);
  J(2,1) = 2.0 * (        k * i / n2rnr - k * i * atn32);

  J(0,2) = 2.0 * (        i * j / n2rnr - i * j * atn32);
  J(1,2) = 2.0 * (atn12 + j * j / n2rnr - j * j * atn32);
  J(2,2) = 2.0 * (        k * j / n2rnr - k * j * atn32);

  J(0,3) = 2.0 * (        i * k / n2rnr - i * k * atn32);
  J(1,3) = 2.0 * (        j * k / n2rnr - j * k * atn32);
  J(2,3) = 2.0 * (atn12 + k * k / n2rnr - k * k * atn32);

  return J * jacobianNormalizeV (V);
}

/**
   Adapted from http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion
   This is both more elegant and better conditioned than most other
   methods.  However, it costs a bit more because of the three extra
   calls to sqrt().

   <p>Warning!  This function will only return a quaternion with a positive value
   in V[0].  All rotations can be expressed this way, but this also means that the
   output of this function may have opposite sign to an equivalent quaternion
   input to V2R().
**/
inline fl::Vector<double>
R2V (const fl::Matrix<double> & R)
{
# if defined (_MSC_VER)
#   define copysign _copysign
# endif

  fl::Vector<double> V (4);
  V[0] =           sqrt (std::max (0.0, 1.0 + R(0,0) + R(1,1) + R(2,2))) / 2.0;
  V[1] = copysign (sqrt (std::max (0.0, 1.0 + R(0,0) - R(1,1) - R(2,2))) / 2.0, R(2,1) - R(1,2));  // std::copysign() should exist across all platforms
  V[2] = copysign (sqrt (std::max (0.0, 1.0 - R(0,0) + R(1,1) - R(2,2))) / 2.0, R(0,2) - R(2,0));
  V[3] = copysign (sqrt (std::max (0.0, 1.0 - R(0,0) - R(1,1) + R(2,2))) / 2.0, R(1,0) - R(0,1));
  return V;
}

/**
   Convert from rotation matrix to Rodriguez vector
**/
inline fl::Vector<double>
R2D (const fl::Matrix<double> & R)
{
  fl::Vector<double> D (3);
  D[0] = R(2,1) - R(1,2);
  D[1] = R(0,2) - R(2,0);
  D[2] = R(1,0) - R(0,1);
  double cosine = (R(0,0) + R(1,1) + R(2,2) - 1.0) / 2.0;
  cosine = std::min ( 1.0, cosine);
  cosine = std::max (-1.0, cosine);
  double theta = acos (cosine);
  double sine = sin (theta);
  if (sine > 1e-10)
  {
	D *= theta / (2.0 * sine);
  }
  else  // Use an approximation
  {
	if (cosine > 0)  // close to zero change in orientation
	{
	  D.clear ();
	}
	else  // nearly 180 degrees of change in orientation
	{
	  double cosine1 = 1.0 - cosine;
	  D[0] = theta * sqrt (std::max (0.0, (R(0,0) - cosine) / cosine1));
	  D[1] = theta * sqrt (std::max (0.0, (R(1,1) - cosine) / cosine1)) * (R(0,1) > 0.0 ? 1.0 : -1.0);
	  D[2] = theta * sqrt (std::max (0.0, (R(2,2) - cosine) / cosine1)) * (R(0,2) > 0.0 ? 1.0 : -1.0);
	}
  }
  return D;
}

/**
   Convert from Rodriguez vector to rotation matrix
**/
inline fl::Matrix<double>
D2R (const fl::Vector<double> & D)
{
  fl::Matrix<double> R (3, 3);
  double theta = D.norm (2);
  if (theta < 1e-10)
  {
	R.identity ();
	return R;
  }
  fl::Vector<double> d;
  d.copyFrom (D);
  d /= theta;
  double cosine = cos (theta);
  double sine = sin (theta);
  double cosine1 = 1.0 - cosine;
  R.clear ();
  R(0,0) = cosine;
  R(0,1) = -d[2] * sine;
  R(0,2) =  d[1] * sine;
  R(1,0) =  d[2] * sine;
  R(1,1) = cosine;
  R(1,2) = -d[0] * sine;
  R(2,0) = -d[1] * sine;
  R(2,1) =  d[0] * sine;
  R(2,2) = cosine;
  return R + d * ~d * cosine1;
}

inline fl::Vector<double>
D2V (const fl::Vector<double> & D)
{
  double theta = D.norm (2);
  fl::Vector<double> V (4);
  if (theta > 1e-10)
  {
	V[0] = cos (theta / 2.0);
	V.region (1) = D * (sin (theta / 2.0) / theta);
	V.normalize ();
  }
  else
  {
	V.clear ();
	V[0] = 1.0;
  }
  return V;
}

inline fl::Matrix<double>
jacobianD2V (const fl::Vector<double> & D)
{
  fl::Matrix<double> J (4, 3);

  double aa = D.sumSquares ();
  double a = sqrt (aa);
  if (aa < 1e-10)
  {
	J.clear ();
	J(1,0) = 0.5;
	J(2,1) = 0.5;
	J(3,2) = 0.5;
	return J;
  }

  double a2 = 2.0 * a;
  double c = cos (a / 2.0);
  double s = sin (a / 2.0);
  double cs = c / a2 - s / aa;

  double & x = D[0];
  double & y = D[1];
  double & z = D[2];
  double xx = x * x;
  double xy = x * y;
  double xz = x * z;
  double yy = y * y;
  double yz = y * z;
  double zz = z * z;

  J(0,0) = (-s * x) / 2.0;
  J(0,1) = (-s * y) / 2.0;
  J(0,2) = (-s * z) / 2.0;
  J(1,0) = c * xx / a2 + s * (1.0 - xx / aa);
  J(1,1) = cs * xy;
  J(1,2) = cs * xz;
  J(2,0) = cs * xy;
  J(2,1) = c * yy / a2 + s * (1.0 - yy / aa);
  J(2,2) = cs * yz;
  J(3,0) = cs * xz;
  J(3,1) = cs * yz;
  J(3,2) = c * zz / a2 + s * (1.0 - zz / aa);
  J /= a;

  return J;
}

inline fl::Matrix<double>
XYZ2R (const fl::Vector<double> & E)
{
  double sx = sin (E[0]);
  double sy = sin (E[1]);
  double sz = sin (E[2]);
  double cx = cos (E[0]);
  double cy = cos (E[1]);
  double cz = cos (E[2]);

  fl::Matrix<double> R (3, 3);
  R(0,0) = cy * cz;
  R(1,0) = cy * sz;
  R(2,0) = -sy;
  R(0,1) = sx * sy * cz - cx * sz;
  R(1,1) = sx * sy * sz + cx * cz;
  R(2,1) = sx * cy;
  R(0,2) = cx * sy * cz + sx * sz;
  R(1,2) = cx * sy * sz - sx * cz;
  R(2,2) = cx * cy;

  return R;
}

inline fl::Vector<double>
R2XYZ (const fl::Matrix<double> & R)
{
  fl::Vector<double> E (3);

  // Trap singularities
  if (1.0 - R(2,0) < 1e-10)
  {
	E[0] = atan2 (-R(0,1), R(1,1));
	E[1] = -M_PI / 2;  // R(2,0) ~= 1 --> sy ~= -1 --> asin (sy) ~= -pi/2
	E[2] = 0;  // angle around z is redundant with angle around x
  }
  else if (1.0 + R(2,0) < 1e-10)
  {
	E[0] = atan2 (R(0,1), R(1,1));
	E[1] = M_PI / 2;
	E[2] = 0;
  }
  else
  {
	E[0] = atan2 (R(2,1), R(2,2));
	E[1] = asin (-R(2,0));
	E[2] = atan2 (R(1,0), R(0,0));
  }
  return E;
}

/**
   Decompose the upper-left 2x2 or 3x3 portion of an affine transformation
   into its component flip, scaling, skew and rotation.
**/
template<class T>
inline void
Decompose (const fl::MatrixAbstract<T> & A, T & flip, fl::Vector<T> & scale, fl::Matrix<T> & skew, fl::Matrix<T> & rotation)
{
  int r = A.rows ();
  int c = A.columns ();
  if (r != c  ||  r < 2  ||  r > 3) throw "Decompose only supports 2x2 and 3x3 matrices.";

  // Find determinant
  T det;
  if (r == 2)
  {
	det = A(0,0) * A(1,1) - A(0,1) * A(1,0);
  }
  else
  {
	const T & a = A(0,0);
	const T & b = A(0,1);
	const T & c = A(0,2);
	const T & d = A(1,0);
	const T & e = A(1,1);
	const T & f = A(1,2);
	const T & g = A(2,0);
	const T & h = A(2,1);
	const T & i = A(2,2);
	det = a * e * i + b * f * g + c * d * h - a * f * h - b * d * i - c * e * g;
  }
  if (std::abs (det) < std::numeric_limits<T>::epsilon ()) throw "Decompose: matrix is nearly singular";

  fl::Matrix<T> U;
  fl::Matrix<T> D;
  fl::Matrix<T> VT;
  gesvd (A / det, U, D, VT);

  if (det < 0)
  {
	flip = -1;
	det *= -1;
  }
  else flip = 1;

  rotation = U * VT;

  skew = U * fl::MatrixDiagonal<T> (D) * ~U;

  scale.resize (r);
  scale[0] = skew(0,0);
  scale[1] = skew(1,1);
  skew.row (0) /= scale[0];
  skew.row (1) /= scale[1];
  if (r == 3)
  {
	scale[2] = skew(2,2);
	skew.row (2) /= scale[2];
  }
  scale *= det;
}


#endif
