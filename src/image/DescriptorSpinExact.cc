#include "fl/descriptor.h"
#include "fl/pi.h"


using namespace std;
using namespace fl;


// class DescriptorSpinExact --------------------------------------------------

static const float hsqrt2 = sqrt (2.0) / 2.0;  // "half square root of 2".

DescriptorSpinExact::DescriptorSpinExact (int binsRadial, int binsIntensity, float supportRadial, float supportIntensity)
{
  this->binsRadial       = binsRadial;
  this->binsIntensity    = binsIntensity;
  this->supportRadial    = supportRadial;
  this->supportIntensity = supportIntensity;
}

class PointZ : public Point
{
public:
  PointZ () {}
  PointZ (float x, float y, float z) : Point (x, y) {this->z = z;}
  float z;
};

typedef vector<PointZ> Polygon;

inline ostream &
operator << (ostream & stream, const PointZ & p)
{
  stream << "(" << p.x << " " << p.y << " " << p.z << ")";
  return stream;
}

inline ostream &
operator << (ostream & stream, const Polygon & p)
{
  for (int i = 0; i < p.size (); i++)
  {
	cerr << p[i];
  }
  return stream;
}

inline void
chopIntensity (Polygon & p, vector<Polygon> & polygons, const float minIntensity, const float quantum, const int lastBin)
{
  // Sort points by intensity
  bool clockwise = false;
  if (p[0].z > p[1].z)
  {
	clockwise = ! clockwise;
	PointZ temp = p[0];
	p[0] = p[1];
	p[1] = temp;
  }
  if (p[1].z > p[2].z)
  {
	clockwise = ! clockwise;
	PointZ temp = p[1];
	p[1] = p[2];
	p[2] = temp;
  }
  if (p[0].z > p[1].z)
  {
	clockwise = ! clockwise;
	PointZ temp = p[0];
	p[0] = p[1];
	p[1] = temp;
  }

  float dx2 = p[2].x - p[0].x;
  float dx1 = p[1].x - p[0].x;

  float dy2 = p[2].y - p[0].y;
  float dy1 = p[1].y - p[0].y;

  float dz2 = p[2].z - p[0].z;
  float dz1 = p[1].z - p[0].z;

  PointZ b = p[0];  // Base along the path with two segments.

  Polygon q;
  q.push_back (p[0]);
  PointZ n1;
  PointZ n2;
  int d = (int) ((p[0].z - minIntensity) / quantum);
  d = d >? 0;
  d = d <? lastBin;
  float z = d * quantum + minIntensity;
  bool passedMiddle = false;
  while (true)
  {
	z += quantum;
	if (d >= lastBin)
	{
	  z = 1e38;  // Extremely high altitude, to ensure all values fall below it.
	}

	float t1 = dz1 == 0 ? 1.0 : (z - b.z) / dz1;
	if (t1 < 0.9999)  // Read "t1 < 1.0".  Just allowing a little numerical slack here.
	{
	  n1.x = dx1 * t1 + b.x;
	  n1.y = dy1 * t1 + b.y;
	}
	else if (! passedMiddle)
	{
	  passedMiddle = true;
	  dx1 = p[2].x - p[1].x;
	  dy1 = p[2].y - p[1].y;
	  dz1 = p[2].z - p[1].z;
	  b = p[1];
	  t1 = dz1 == 0 ? 1.0 : (z - b.z) / dz1;
	  if (t1 < 0.0001)  // Read "t1 == 0"
	  {
		n1 = p[1];
	  }
	  else
	  {
		if (clockwise)
		{
		  q.insert (q.begin (), p[1]);
		}
		else
		{
		  q.push_back (p[1]);
		}
		n1.x = dx1 * t1 + b.x;
		n1.y = dy1 * t1 + b.y;
	  }
	}

	float t2 = dz2 == 0 ? 1.0 : (z - p[0].z) / dz2;
	if (t2 < 0.9999)
	{
	  n2.x = dx2 * t2 + p[0].x;
	  n2.y = dy2 * t2 + p[0].y;
	}
	else
	{
	  q.push_back (p[2]);
	  q[0].z = d;
	  polygons.push_back (q);
	  return;
	}

	if (clockwise)
	{
	  q.push_back (n2);
	  q.push_back (n1);
	  q[0].z = d;
	  polygons.push_back (q);
	  q.clear ();
	  q.push_back (n1);
	  q.push_back (n2);
	}
	else
	{
	  q.push_back (n1);
	  q.push_back (n2);
	  q[0].z = d;
	  polygons.push_back (q);
	  q.clear ();
	  q.push_back (n2);
	  q.push_back (n1);
	}

	d++;
  }
}

inline float
length (const Point & a, const Point & b)
{
  float dx = a.x - b.x;
  float dy = a.y - b.y;
  return sqrt (dx * dx + dy * dy);
}

inline float
areaTriangle (const Point & p0, const Point & p1, const Point & p2)
{
  float a = length (p0, p1);
  float b = length (p1, p2);
  float c = length (p2, p0);
  float s = (a + b + c) / 2.0;
  return sqrt (s * (s - a) * (s - b) * (s - c) >? 0); // Heron's formula
}

inline float
areaArc (const Point & p, const Point & q, const Point & center, float radius)
{
  float a1 = atan2 (p.y - center.y, p.x - center.x);
  float a2 = atan2 (q.y - center.y, q.x - center.x);
  float da = fabs (a1 - a2);
  if (da > PI)
  {
	da = 2 * PI - da;
  }
  return radius * radius * da / 2.0;
}

inline float
areaEdge (const Point & p, const Point & q, const Point & center, float radius)
{
  // Find orientation of (p, q, center)
  float d1 = (p.x - center.x) * (q.y - center.y);
  float d2 = (p.y - center.y) * (q.x - center.x);
  float sign;
  if (d1 == d2)
  {
	sign = 0;
  }
  else
  {
	sign = d1 > d2 ? +1 : -1;
  }

  // Find intersections with circle
  // Solve for roots of t in ||(q - p) t + p - center|| = radius
  float xd = q.x - p.x;
  float yd = q.y - p.y;
  float xe = p.x - center.x;
  float ye = p.y - center.y;
  float a = xd * xd + yd * yd;
  float b = 2 * (xd * xe + yd * ye);
  float c = xe * xe + ye * ye - radius * radius;
  float b4ac = b * b - 4 * a * c;
  if (b4ac <= 0)
  {
	// Line supporting edge either intersects circle at exactly one point,
	// or does not intersect circle at all.  This means the edge is entirely
	// outside circle.
	return sign * areaArc (p, q, center, radius);
  }
  b4ac = sqrt (b4ac);
  float t1 = (-b - b4ac) / (2 * a);
  float t2 = (-b + b4ac) / (2 * a);

  // Determine area under each portion of segment
  if (t1 <= 0)
  {
	if (t2 <= 0)
	{
	  // Edge is entirely outside circle
	  return sign * areaArc (p, q, center, radius);
	}
	else if (t2 >= 1)
	{
	  // Edge is entirely inside circle
	  return sign * areaTriangle (p, q, center);
	}
	else  // t2 in (0, 1)
	{
	  // p is inside and q is outside
	  Point c2 (xd * t2 + p.x, yd * t2 + p.y);
	  return   sign
		     * (  areaTriangle (p, c2, center)
				+ areaArc (c2, q, center, radius));
	}
  }
  else if (t1 >= 1)
  {
	// Edge is entirely outside circle
	return sign * areaArc (p, q, center, radius);
  }
  else  // t1 in (0, 1)
  {
	Point c1 (xd * t1 + p.x, yd * t1 + p.y);
	if (t2 >= 1)
	{
	  // p is outside and q is inside
	  return   sign
		     * (  areaArc (p, c1, center, radius)
				+ areaTriangle (c1, q, center));
	}
	else  // t2 in (t1, 1)
	{
	  // p and q are both outside
	  Point c2 (xd * t2 + p.x, yd * t2 + p.y);
	  return   sign
		     * (  areaArc (p, c1, center, radius)
				+ areaTriangle (c1, c2, center)
				+ areaArc (c2, q, center, radius));
	}
  }
}

void
DescriptorSpinExact::doBinning (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float minIntensity, float quantum, float binRadius, Vector<float> & result)
{
  // Extend area of coverage to ensure all intensity patches are considered.
  x1 = 0 >? x1 - 1;
  y1 = 0 >? y1 - 1;
  x2 = x2 <? image.width - 2;
  y2 = y2 <? image.height - 2;

  // Since the upper left pixel center represents the entire group of four
  // pixel centers which define an intensity patch, there is an asymmetry
  // in how pixels should be evaluated for inclusion in the support for the
  // spin image.  The correct compensation for this asymmetry is to shift
  // the center up and left by half a pixel.  This is the same as shifting
  // the upper left pixel position into the center of the intensity patch.
  float scx = point.x - 0.5;  // "shift center x"
  float scy = point.y - 0.5;  // "shift center y"

  ImageOf<float> that (image);
  result.resize (binsRadial * binsIntensity);
  result.clear ();

  for (int x = x1; x <= x2; x++)
  {
	float dx = x - scx;
	for (int y = y1; y <= y2; y++)
	{
	  float dy = y - scy;
	  float radius = sqrt (dx * dx + dy * dy);
	  if (radius < width)
	  {
		// Chop pixel into two triangles, then chop each triangle up
		// according to intensity level.  Each piece must have an associated
		// intensity bin.
		vector<Polygon> polygons;
		//polygons.reserve (2 * binsIntensity);  // Doesn't seem to improve performance.
		Polygon polygon;
		//polygon.reserve (3);
		PointZ p1 (x,     y,     that (x,     y));
		PointZ p2 (x + 1, y,     that (x + 1, y));
		PointZ p3 (x,     y + 1, that (x,     y + 1));
		PointZ p4 (x + 1, y + 1, that (x + 1, y + 1));
		polygon.push_back (p1);
		polygon.push_back (p2);
		polygon.push_back (p3);
		chopIntensity (polygon, polygons, minIntensity, quantum, binsIntensity - 1);
		polygon.clear ();
		polygon.push_back (p3);
		polygon.push_back (p2);
		polygon.push_back (p4);
		chopIntensity (polygon, polygons, minIntensity, quantum, binsIntensity - 1);

		// Chop pieces into triangles.
		vector<Polygon> triangles;
		triangles.reserve (polygons.size () * 2);  // Does in fact improve performance
		for (int i = 0; i < polygons.size (); i++)
		{
		  Polygon & p = polygons[i];
		  for (int j = 2; j < p.size (); j++)
		  {
			Polygon q;
			q.push_back (p[0]);
			q.push_back (p[j - 1]);
			q.push_back (p[j]);
			triangles.push_back (q);
		  }
		}

		// Process each triangular piece, assigning portion of area to
		// appropriate radial bin
		vector<Polygon>::iterator t;
		for (t = triangles.begin (); t < triangles.end (); t++)
		{
		  int r = (int) ((radius - hsqrt2) / binRadius);
		  r = r >? 0;
		  int d = (int) (*t)[0].z;
		  float total = areaTriangle ((*t)[0], (*t)[1], (*t)[2]);
		  float consumed = 0;
		  for (; r < binsRadial  &&  (total - consumed) > 1e-6; r++)
		  {
			// Find area under each edge
			float radius = (r + 1) * binRadius;  // Changes meaning of radius in this scope
			float area = 0;
			area += areaEdge ((*t)[0], (*t)[1], point, radius);
			area += areaEdge ((*t)[1], (*t)[2], point, radius);
			area += areaEdge ((*t)[2], (*t)[0], point, radius);

			result[r * binsIntensity + d] += area - consumed;
			consumed = area;
		  }
		}
	  }
	}
  }
}
