#include "fl/Matrix.tcc"
#include "fl/MatrixComplex.tcc"


using namespace fl;
using namespace std;


template class MatrixAbstract<complex double>;
template class Matrix<complex double>;
template class MatrixTranspose<complex double>;
template class MatrixRegion<complex double>;
