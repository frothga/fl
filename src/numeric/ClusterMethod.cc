#include "fl/cluster.h"
#include "fl/factory.h"


using namespace fl;
using namespace std;


// class ClusterMethod --------------------------------------------------------

Factory<ClusterMethod>::productMap Factory<ClusterMethod>::products;

void
ClusterMethod::read (istream & stream)
{
}

void
ClusterMethod::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
