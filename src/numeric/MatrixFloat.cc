#include "fl/Matrix.tcc"
#include "fl/factory.h"


using namespace fl;


template class MatrixAbstract<float>;
template class Matrix<float>;
template class MatrixTranspose<float>;
template class MatrixRegion<float>;

Factory<MatrixAbstract<float> >::productMap Factory<MatrixAbstract<float> >::products;
