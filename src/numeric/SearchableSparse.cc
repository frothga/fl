#include "fl/search.h"

#include <vector>


using namespace fl;
using namespace std;


// For debugging
//#include <iostream>
//using namespace std;


// class SearchableSparse -----------------------------------------------------

void
SearchableSparse::cover ()
{
  MatrixSparse<bool> interaction = this->interaction ();

  const int m = dimension ();  // == interaction.rows ()
  const int n = interaction.columns ();

  parameters.resize (0, 0);
  parms.clear ();

  vector<int> columns (n);
  for (int i = 0; i < n; i++)
  {
	columns[i] = i;
  }

  while (columns.size ())
  {
	// Add column to parameters
	int j = parameters.columns ();
	parameters.resize (m, j + 1);
	parms.resize (j + 1);
	map<int,int> & Cj = (*parameters.data)[j];

	for (int i = columns.size () - 1; i >= 0; i--)
	{
	  int c = columns[i];

	  // Determine if the selected column is compatible with column j of parameters
	  bool compatible = true;
	  map<int,bool> & Ci = (*interaction.data)[c];
	  map<int,int>::iterator ij = Cj.begin ();
	  map<int,bool>::iterator ii = Ci.begin ();
	  while (ij != Cj.end ()  &&  ii != Ci.end ())
	  {
		if (ij->first == ii->first)
		{
		  compatible = false;
		  break;
		}
		else if (ij->first > ii->first)
		{
			ii++;
		}
		else  // ij->first < ii->first
		{
			ij++;
		}
	  }

	  // If yes, update column information
	  if (compatible)
	  {
		ij = Cj.begin ();
		ii = Ci.begin ();
		while (ii != Ci.end ())
		{
		  ij = Cj.insert (ij, make_pair (ii->first, c + 1));  // Offset column indices by 1 to deal with sparse representation.
		  ii++;
		}
		parms[j].push_back (c);

		columns.erase (columns.begin () + i);
	  }
	}
  }
}

void
SearchableSparse::jacobian (const Vector<double> & point, Matrix<double> & result, const Vector<double> * currentValue)
{
  const int m = dimension ();
  const int n = point.rows ();

  result.resize (m, n);
  result.clear ();

  Vector<double> oldValue;
  if (currentValue)
  {
	oldValue = *currentValue;
  }
  else
  {
	value (point, oldValue);
  }

  Vector<double> column (m);
  Vector<double> p (n);
  for (int i = 0; i < parms.size (); i++)
  {
	vector<int> & parmList = parms[i];

	p.clear ();
	for (int j = 0; j < parmList.size (); j++)
	{
	  int k = parmList[j];

	  double h = perturbation * fabs (point[k]);
	  if (h == 0)
	  {
		h = perturbation;
	  }
	  p[k] = h;
	}

	value (point + p, column);

	map<int,int> & C = (*parameters.data)[i];
	map<int,int>::iterator j = C.begin ();
	while (j != C.end ())
	{
	  int r = j->first;
	  int c = j->second - 1;  // Offset by 1 due to needs of sparse representation.
	  result(r,c) = (column[r] - oldValue[r]) / p[c];
	  j++;
	}
  }
}

void
SearchableSparse::jacobian (const Vector<double> & point, MatrixSparse<double> & result, const Vector<double> * currentValue)
{
  const int m = dimension ();
  const int n = point.rows ();

  result.resize (m, n);
  result.clear ();

  Vector<double> oldValue;
  if (currentValue)
  {
	oldValue = *currentValue;
  }
  else
  {
	value (point, oldValue);
  }

  Vector<double> column (m);
  Vector<double> p (n);
  for (int i = 0; i < parms.size (); i++)
  {
	vector<int> & parmList = parms[i];

	p.clear ();
	for (int j = 0; j < parmList.size (); j++)
	{
	  int k = parmList[j];

	  double h = perturbation * fabs (point[k]);
	  if (h == 0)
	  {
		h = perturbation;
	  }
	  p[k] = h;
	}

	value (point + p, column);

	map<int,int> & C = (*parameters.data)[i];
	map<int,int>::iterator j = C.begin ();
	while (j != C.end ())
	{
	  int r = j->first;
	  int c = j->second - 1;
	  result.set (r, c, (column[r] - oldValue[r]) / p[c]);
	  j++;
	}
  }
}
