#include "fl/LevenbergMarquardtSparseBK.tcc"


using namespace fl;


const float LevenbergMarquardtSparseBK<float>::epsilon = FLT_EPSILON;
const float LevenbergMarquardtSparseBK<float>::minimum = FLT_MIN;
template class LevenbergMarquardtSparseBK<float>;
