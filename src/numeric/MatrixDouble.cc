#include "fl/Matrix.tcc"
#include "fl/factory.h"


using namespace fl;


template class MatrixAbstract<double>;
template class Matrix<double>;
template class MatrixTranspose<double>;
template class MatrixRegion<double>;

Factory<MatrixAbstract<double> >::productMap Factory<MatrixAbstract<double> >::products;
