#include "fl/Matrix.tcc"
#include "fl/factory.h"


using namespace fl;


template class MatrixAbstract<int>;
template class Matrix<int>;
template class MatrixTranspose<int>;
template class MatrixRegion<int>;

Factory<MatrixAbstract<int> >::productMap Factory<MatrixAbstract<int> >::products;
