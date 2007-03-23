/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2, 1.4 thru 1.6 Copyright 2005 Sandia Corporation.
Revisions 1.8 and 1.9       Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 11:38:05  Fred
Correct which revisions are under Sandia copyright.

Revision 1.8  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.7  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 03:41:12  Fred
Move the file name LICENSE up to previous line, for better symmetry with UIUC
notice.

Revision 1.5  2005/10/08 18:41:56  Fred
Update revision history and add Sandia copyright notice.

Revision 1.4  2005/08/07 03:10:08  Fred
GCC 3.4 compilability fix: explicitly specify "this" for inherited member
variables in a template.

Revision 1.3  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/12 04:59:59  rothgang
Use std versions of pow, sqrt, and fabs so that choice of type specific version
will be automatic when template is instantiated.  IE: std contains type
overloaded versions of the functions rather than separately named functions.

Revision 1.1  2004/04/19 21:22:43  rothgang
Create template versions of Search classes.
-------------------------------------------------------------------------------
*/


#ifndef fl_searchable_sparse_tcc
#define fl_searchable_sparse_tcc


#include "fl/search.h"

#include <vector>


namespace fl
{
  // class SearchableSparse ---------------------------------------------------
  template<class T>
  void
  SearchableSparse<T>::cover ()
  {
	MatrixSparse<bool> interaction = this->interaction ();

	const int m = this->dimension ();  // == interaction.rows ()
	const int n = interaction.columns ();

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
  SearchableSparse<T>::jacobian (const Vector<T> & point, Matrix<T> & result, const Vector<T> * currentValue)
  {
	const int m = this->dimension ();
	const int n = point.rows ();

	result.resize (m, n);
	result.clear ();

	Vector<T> oldValue;
	if (currentValue)
	{
	  oldValue = *currentValue;
	}
	else
	{
	  value (point, oldValue);
	}

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
	const int m = this->dimension ();
	const int n = point.rows ();

	result.resize (m, n);
	result.clear ();

	Vector<T> oldValue;
	if (currentValue)
	{
	  oldValue = *currentValue;
	}
	else
	{
	  value (point, oldValue);
	}

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
