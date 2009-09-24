# Author: Fred Rothganger
# Copyright 2009 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


add_library (flNumeric
  ../../include/fl/math.h
  ../../include/fl/pi.h
  zroots.cc

  # Metrics, Distances, Measures, etc.
  ../../include/fl/metric.h
  Metric.cc

  # Matrices
  ../../include/fl/matrix.h
  ../../include/fl/Matrix.tcc
  ../../include/fl/MatrixDiagonal.tcc
  ../../include/fl/MatrixFixed.tcc
  ../../include/fl/MatrixFixed22.tcc
  ../../include/fl/MatrixIdentity.tcc
  ../../include/fl/MatrixPacked.tcc
  ../../include/fl/MatrixSparse.tcc
  ../../include/fl/Vector.tcc
  #   Double
  MatrixDouble.cc
  VectorDouble.cc
  MatrixFixedDouble22.cc
  MatrixFixedDouble33.cc
  MatrixPackedDouble.cc
  MatrixIdentityDouble.cc
  MatrixDiagonalDouble.cc
  MatrixSparseDouble.cc
  #   Float
  MatrixFloat.cc
  VectorFloat.cc
  MatrixFixedFloat22.cc
  MatrixFixedFloat33.cc
  MatrixPackedFloat.cc
  MatrixIdentityFloat.cc
  MatrixDiagonalFloat.cc
  MatrixSparseFloat.cc
  #   Complex Double
  ../../include/fl/MatrixComplex.tcc
  MatrixComplexDouble.cc
  VectorComplexDouble.cc
  #   Boolean
  ../../include/fl/MatrixBool.tcc
  ../../include/fl/MatrixSparseBool.tcc
  MatrixBool.cc
  VectorBool.cc
  MatrixSparseBool.cc
  #   Integer
  MatrixInt.cc
  VectorInt.cc
  MatrixSparseInt.cc

  # Lapack bridge
  ../../include/fl/lapack.h
  ../../include/fl/lapackproto.h
  ../../include/fl/lapackprotod.h
  ../../include/fl/lapackprotos.h
  #   Double
  dgeev.cc
  dgelss.cc
  dgelsd.cc
  dgesvd.cc
  dgetrf.cc
  dspev.cc
  dsyev.cc
  dsygv.cc
  #   Float
  sgeev.cc
  sgelss.cc
  sgelsd.cc
  sgesvd.cc
  sgetrf.cc
  sspev.cc
  ssyev.cc
  ssygv.cc

  # Search
  ../../include/fl/search.h
  ../../include/fl/AnnealingAdaptive.tcc
  ../../include/fl/LineSearch.tcc
  ../../include/fl/LevenbergMarquardt.tcc
  ../../include/fl/LevenbergMarquardtSparseBK.tcc
  ../../include/fl/SearchableNumeric.tcc
  ../../include/fl/SearchableSparse.tcc
  #   Double
  AnnealingAdaptiveDouble.cc
  LevenbergMarquardtDouble.cc
  LevenbergMarquardtSparseBKDouble.cc
  SearchableNumericDouble.cc
  SearchableSparseDouble.cc
  #   Float
  AnnealingAdaptiveFloat.cc
  LevenbergMarquardtFloat.cc
  LevenbergMarquardtSparseBKFloat.cc
  SearchableNumericFloat.cc
  SearchableSparseFloat.cc

  # Clustering
  ../../include/fl/cluster.h
  ClusterMethod.cc
  KMeans.cc
  KMeansParallel.cc
  Kohonen.cc
  Agglomerate.cc

  # Neural Networks
  ../../include/fl/neural.h
  NeuralNetwork.cc
  NeuralNetworkBackprop.cc
  NeuralNetworkVector.cc

  # Dimensionality reduction
  ../../include/fl/reduce.h
  DimensionalityReduction.cc
  PCA.cc
  MDA.cc
)

install (TARGETS flNumeric
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

add_executable (testNumeric
  test.cc
)

target_link_libraries (testNumeric
  flNumeric
  flBase
  ${LAPACK_LIBRARIES}
  ${GCC_LIBRARIES}
)
