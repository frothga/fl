#include "fl/Matrix.tcc"
#include "fl/MatrixBool.tcc"
#include "fl/factory.h"


using namespace fl;


int MatrixAbstract<bool>::displayWidth = 1;

template class MatrixAbstract<bool>;
template class Matrix<bool>;
template class MatrixTranspose<bool>;
template class MatrixRegion<bool>;

Factory<MatrixAbstract<bool> >::productMap Factory<MatrixAbstract<bool> >::products;
