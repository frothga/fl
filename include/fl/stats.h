#ifndef fl_stats_h
#define fl_stats_h


#include <vector>
#include <ios>


namespace fl
{
  class Stats
  {
  public:
	void add (float value)
	{
	  values.push_back (value);
	}

	void dump (std::ostream & out, int binCount = 0)
	{
	  float min = INFINITY;
	  float max = -INFINITY;
	  float ave = 0;
	  for (int i = 0; i < values.size (); i++)
	  {
		ave += values[i];
		min = std::min (min, values[i]);
		max = std::max (max, values[i]);
	  }
	  ave /= values.size ();

	  float std = 0;
	  for (int i = 0; i < values.size (); i++)
	  {
		float t = values[i] - ave;
		std += t * t;
	  }
	  std = sqrtf (std / values.size ());

	  if (binCount <= 0)
	  {
		out << values.size () << " " << ave << " " << std << " " << min << " " << max << std::endl;
	  }
	  else
	  {
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
	}

	std::vector<float> values;
  };
}

#endif
