#include "fl/interest.h"


using namespace std;
using namespace fl;


// class InterestOperator -----------------------------------------------------

void
InterestOperator::run (const Image & image, vector<PointInterest> & result)
{
  multiset<PointInterest> temp;
  run (image, temp);

  result.resize (temp.size ());
  multiset<PointInterest>::iterator it;
  int i = 0;
  for (it = temp.begin (); it != temp.end (); it++)
  {
	result[i++] = *it;
  }
}

