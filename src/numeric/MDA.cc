#include "fl/reduce.h"
#include "fl/lapacks.h"


using namespace std;
using namespace fl;


// class MDA ------------------------------------------------------------------

MDA::MDA (istream & stream)
{
  read (stream);
}

/**
   This initial implementation assumes that classAssignments numbers classes
   contiquously from 0 to c-1, and that no other integers besides [0,c-1]
   exist in the vector.  A more general version could be written to analyze
   and remap the class numbers.
**/
void
MDA::analyze (const vector< Vector<float> > & data, const vector<int> & classAssignments)
{
  cerr << "Warning: MDA has not been properly tested yet." << endl;

  // Find number of distinct classes
  vector<int> classSizes;
  for (int i = 0; i < classAssignments.size (); i++)
  {
	int c = classAssignments[i];
	if (c >= classSizes.size ())
	{
	  classSizes.resize (c + 1, 0);
	}
	classSizes[c]++;
  }
  int c = classSizes.size ();  // number of classes
  int d = data[0].rows ();  // Dimension of full space.
  if (c < 2)
  {
	throw "Must have at least two classes to perform MDA.";
  }
  if (d < c)
  {
	throw "Dimension of space must be at least as large as number of classes.";
  }

  // Find the mean of each class
  Matrix<float> means (d, c);
  means.clear ();
  for (int i = 0; i < data.size (); i++)
  {
	means.column (classAssignments[i]) += data[i];
  }
  Vector<float> mean;
  mean.clear ();
  for (int i = 0; i < c; i++)
  {
	mean += means.column (i);
	means.column (i) /= classSizes[i];
  }
  mean /= data.size ();

  // Compute Sw, the total within-class covariance matrix
  Matrix<float> Sw (d, d);
  Sw.clear ();
  for (int i = 0; i < data.size (); i++)
  {
	Vector<float> delta = data[i] - means.column (classAssignments[i]);
	Sw += delta * ~delta;
  }

  // Compute Sb, the between-class covariance matrix
  Matrix<float> Sb (d, d);
  Sb.clear ();
  for (int i = 0; i < c; i++)
  {
	Vector<float> delta = means.column (i) - mean;
	Sb += delta * ~delta;
  }

  // Solve the generalized eigenvalue problem
  Vector<float> eigenvalues;
  Matrix<float> eigenvectors;
  sygv (Sb, Sw, eigenvalues, eigenvectors);

  map<float, int> sorted;
  for (int i = 0; i < eigenvalues.rows (); i++)
  {
	sorted.insert (make_pair (fabsf (eigenvalues[i]), i));
  }

  W.resize (c-1, d);
  map<float, int>::reverse_iterator it = sorted.rbegin ();
  for (int i = 0; i < c-1  &&  it != sorted.rend (); it++, i++)
  {
	W.row (i) = ~ eigenvectors.column (it->second);
  }
}

Vector<float>
MDA::reduce (const Vector<float> & datum)
{
  return W * datum;
}

void
MDA::read (istream & stream)
{
  W.read (stream);
}

void
MDA::write (ostream & stream, bool withName)
{
  W.write (stream, false);
}
