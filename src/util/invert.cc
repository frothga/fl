#include <fl/Matrix.tcc>
#include <fl/lapackd.h>


using namespace std;
using namespace fl;


int
main (int argc, char * argv[])
{
  Matrix<double> A;

  cin >> A;
  cout << !A << endl;

  return 0;
}
