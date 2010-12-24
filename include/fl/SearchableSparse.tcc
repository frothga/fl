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


#ifndef fl_searchable_sparse_tcc
#define fl_searchable_sparse_tcc


#include "fl/search.h"

#include <vector>


namespace fl
{
  // class SearchableSparse ---------------------------------------------------

  template<class T>
  SearchableSparse<T>::SearchableSparse (T perturbation)
  : SearchableNumeric<T> (perturbation)
  {
	coveredDimension = -1;
  }

  template<class T>
  void
  SearchableSparse<T>::cover ()
  {
	MatrixSparse<bool> interaction = this->interaction ();

	const int m = interaction.rows ();
	const int n = interaction.columns ();

	coveredDimension = m;

	parameters.resize (0, 0);
	parms.clear ();

	std::vector<int> columns (n);
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
	  std::map<int,int> & Cj = (*parameters.data)[j];

	  for (int i = columns.size () - 1; i >= 0; i--)
	  {
		int c = columns[i];

		// Determine if the selected column is compatible with column j of parameters
		bool compatible = true;
		std::map<int,bool> & Ci = (*interaction.data)[c];
		std::map<int,int>::iterator ij = Cj.begin ();
		std::map<int,bool>::iterator ii = Ci.begin ();
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
			ij = Cj.insert (ij, std::make_pair (ii->first, c + 1));  // Offset column indices by 1 to deal with sparse representation.
			ii++;
		  }
		  parms[j].push_back (c);

		  columns.erase (columns.begin () + i);
		}
	  }
	}
  }

  template<class T>
  void
  SearchableSparse<T>::gradient (const Vector<T> & point, Vector<T> & result, const Vector<T> * currentValue)
  {
	Vector<T> y;
	if (currentValue) y = *currentValue;
	else              value (point, y);

	MatrixSparse<T> J;
	jacobian (point, J, &y);

	result = ((T) 2) * J.transposeMultiply (y);
  }

  template<class T>
  void
  SearchableSparse<T>::jacobian (const Vector<T> & point, Matrix<T> & result, const Vector<T> * currentValue)
  {
	Vector<T> oldValue;
	if (currentValue)
	{
	  oldValue = *currentValue;
	}
	else
	{
	  value (point, oldValue);
	}

	const int m = oldValue.rows ();
	const int n = point.rows ();

	if (m != coveredDimension) cover ();

	result.resize (m, n);
	result.clear ();

	Vector<T> column (m);
	Vector<T> p (n);
	for (int i = 0; i < parms.size (); i++)
	{
	  std::vector<int> & parmList = parms[i];

	  p.clear ();
	  for (int j = 0; j < parmList.size (); j++)
	  {
		int k = parmList[j];

		T h = this->perturbation * std::fabs (point[k]);
		if (h == 0)
		{
		  h = this->perturbation;
		}
		p[k] = h;
	  }

	  value (point + p, column);

	  std::map<int,int> & C = (*parameters.data)[i];
	  std::map<int,int>::iterator j = C.begin ();
	  while (j != C.end ())
	  {
		int r = j->first;
		int c = j->second - 1;  // Offset by 1 due to needs of sparse representation.
		result(r,c) = (column[r] - oldValue[r]) / p[c];
		j++;
	  }
	}
  }

  template<class T>
  void
  SearchableSparse<T>::jacobian (const Vector<T> & point, MatrixSparse<T> & result, const Vector<T> * currentValue)
  {
	Vector<T> oldValue;
	if (currentValue)
	{
	  oldValue = *currentValue;
	}
	else
	{
	  value (point, oldValue);
	}

	const int m = oldValue.rows ();
	const int n = point.rows ();

	if (m != coveredDimension) cover ();

	result.resize (m, n);
	result.clear ();

	Vector<T> column (m);
	Vector<T> p (n);
	for (int i = 0; i < parms.size (); i++)
	{
	  std::vector<int> & parmList = parms[i];

	  p.clear ();
	  for (int j = 0; j < parmList.size (); j++)
	  {
		int k = parmList[j];

		T h = this->perturbation * std::fabs (point[k]);
		if (h == 0)
		{
		  h = this->perturbation;
		}
		p[k] = h;
	  }

	  value (point + p, column);

	  std::map<int,int> & C = (*parameters.data)[i];
	  std::map<int,int>::iterator j = C.begin ();
	  while (j != C.end ())
	  {
		int r = j->first;
		int c = j->second - 1;
		result.set (r, c, (column[r] - oldValue[r]) / p[c]);
		j++;
	  }
	}
  }
}


#endif
