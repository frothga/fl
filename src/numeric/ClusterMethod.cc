#include "fl/cluster.h"

using namespace fl;
using namespace std;


// class ClusterMethod --------------------------------------------------------

ClusterMethod *
ClusterMethod::factory (std::istream & stream)
{
  string name;
  getline (stream, name);

  if (name == typeid (KMeans).name ())
  {
	return new KMeans (stream);
  }
  else if (name == typeid (KMeansParallel).name ())
  {
	return new KMeansParallel (stream);
  }
  else if (name == typeid (Kohonen).name ())
  {
	return new Kohonen (stream);
  }
  else
  {
	cerr << "Unknown ClusterMethod: " << name << endl;
	return NULL;
  }
}
