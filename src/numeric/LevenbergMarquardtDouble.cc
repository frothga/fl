#include "fl/LevenbergMarquardt.tcc"


using namespace fl;


const double LevenbergMarquardt<double>::epsilon = DBL_EPSILON;
const double LevenbergMarquardt<double>::minimum = DBL_MIN;
template class LevenbergMarquardt<double>;
