#include "fl/Matrix.tcc"


using namespace fl;


int MatrixAbstract<bool>::displayWidth = 1;

template class MatrixAbstract<bool>;
template class Matrix<bool>;
template class MatrixTranspose<bool>;
template class MatrixRegion<bool>;
