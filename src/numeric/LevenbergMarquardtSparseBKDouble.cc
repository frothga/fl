#include "fl/LevenbergMarquardtSparseBK.tcc"


using namespace fl;


const double LevenbergMarquardtSparseBK<double>::epsilon = DBL_EPSILON;
const double LevenbergMarquardtSparseBK<double>::minimum = DBL_MIN;
template class LevenbergMarquardtSparseBK<double>;
