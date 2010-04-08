#include <iostream>
#include <fl/parms.h>


using namespace std;
using namespace fl;


int main (int argc, char * argv[])
{
  cerr << "hello" << endl;

  Parameters parms (argc, argv);

  cerr << parms.getChar ("bob", "bob not found") << endl;


  return 0;
}
