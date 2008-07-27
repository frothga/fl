/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_stats_h
#define fl_stats_h


#include "fl/math.h"

#include <vector>
#include <ostream>


namespace fl
{
  class Stats
  {
  public:
	void add (float value)
	{
	  values.push_back (value);
	}

	void summarize ()
	{
	  min = INFINITY;
	  max = -INFINITY;
	  ave = 0;
	  for (int i = 0; i < values.size (); i++)
	  {
		ave += values[i];
		min = std::min (min, values[i]);
		max = std::max (max, values[i]);
	  }
	  ave /= values.size ();

	  std = 0;
	  for (int i = 0; i < values.size (); i++)
	  {
		float t = values[i] - ave;
		std += t * t;
	  }
	  std = sqrtf (std / values.size ());
	}

	void histogram (std::ostream & out, int binCount = 10)
	{
	  summarize ();
	  float range = (max - min) / binCount;
	  std::vector<int> bins (binCount);
	  for (int i = 0; i < binCount; i++)
	  {
		bins[i] = 0;
	  }
	  for (int i = 0; i < values.size (); i++)
	  {
		bins[(int) floorf ((values[i] - min) / range) <? (binCount - 1)]++;
	  }

	  for (int i = 0; i < binCount; i++)
	  {
		//out << "     [" << min + range * i << "," << min + range * (i+1) << ") = " << bins[i] << endl;
		out << min + range * (i + 0.5f) << " " << bins[i] << std::endl;
	  }
	}

	std::vector<float> values;
	float ave;
	float std;
	float min;
	float max;
  };

  inline std::ostream &
  operator << (std::ostream & out, Stats & stats)
  {
	stats.summarize ();
	out << stats.values.size () << " " << stats.ave << " " << stats.std << " " << stats.min << " " << stats.max;
	return out;
  }
}

#endif
