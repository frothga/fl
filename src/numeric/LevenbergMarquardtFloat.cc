#include "fl/LevenbergMarquardt.tcc"


using namespace fl;


const float LevenbergMarquardt<float>::epsilon = FLT_EPSILON;
const float LevenbergMarquardt<float>::minimum = FLT_MIN;
template class LevenbergMarquardt<float>;
