#include "fl/interest.h"


using namespace std;
using namespace fl;


// class InterestOperator -----------------------------------------------------

void
InterestOperator::run (const Image & image, vector<PointInterest> & result)
{
  multiset<PointInterest> temp;
  run (image, temp);
  result.clear ();
  result.insert (result.end (), temp.begin (), temp.end ());
}

