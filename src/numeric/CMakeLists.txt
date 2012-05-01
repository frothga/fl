# Author: Fred Rothganger
# Copyright 2009, 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


if (CMAKE_USE_PTHREADS_INIT)
  set (maybeGaussianMixtureParallel GaussianMixtureParallel.cc)
endif (CMAKE_USE_PTHREADS_INIT)

if (BLAS_FOUND)
  set (flNumericBLAS
    ../../include/fl/blasproto.h
  )
endif (BLAS_FOUND)

# Note that LAPACK_FOUND implies BLAS_FOUND
if (LAPACK_FOUND)
  set (flNumericLAPACK
    ../../include/fl/lapack.h
    ../../include/fl/lapack.tcc
    ../../include/fl/lapackproto.h
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
  )
  set (maybeSearch
    LevenbergMarquardtDouble.cc
    LevenbergMarquardtFloat.cc
    NewtonRaphsonDouble.cc
    NewtonRaphsonFloat.cc
    ConjugateGradientDouble.cc
    ConjugateGradientFloat.cc
  )
endif (LAPACK_FOUND)

if (FFTW_FOUND)
  set (maybeFourier
    FourierDouble.cc
    FourierFloat.cc
  )
endif (FFTW_FOUND)

add_library (flNumeric
  ../../include/fl/math.h
  ../../include/fl/random.h
  ../../include/fl/stats.h
  random.cc
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
  MatrixComplexDouble.cc
  VectorComplexDouble.cc
  #   Complex Float
  MatrixComplexFloat.cc
  #   Boolean
  ../../include/fl/MatrixBool.tcc
  ../../include/fl/MatrixSparseBool.tcc
  MatrixBool.cc
  VectorBool.cc
  MatrixSparseBool.cc
  #   Integer (system dependent size)
  MatrixInt.cc
  VectorInt.cc
  MatrixSparseInt.cc
  #   uint16_t
  MatrixUint16.cc

  # Lapack bridge
  ${flNumericBLAS}
  ${flNumericLAPACK}

  # FFTW bridge
  ../../include/fl/fourier.h
  ../../include/fl/Fourier.tcc
  ${maybeFourier}

  # Search
  ../../include/fl/search.h
  ../../include/fl/AnnealingAdaptive.tcc
  ../../include/fl/ConjugateGradient.tcc
  ../../include/fl/GradientDescent.tcc
  ../../include/fl/LevenbergMarquardt.tcc
  ../../include/fl/LevenbergMarquardtSparseBK.tcc
  ../../include/fl/LineSearch.tcc
  ../../include/fl/NewtonRaphson.tcc
  ../../include/fl/ParticleSwarm.tcc
  ../../include/fl/SearchableNumeric.tcc
  ../../include/fl/SearchableSparse.tcc
  ${maybeSearch}
  #   Double
  AnnealingAdaptiveDouble.cc
  GradientDescentDouble.cc
  LevenbergMarquardtSparseBKDouble.cc
  LineSearchDouble.cc
  ParticleSwarmDouble.cc
  SearchableNumericDouble.cc
  SearchableSparseDouble.cc
  #   Float
  AnnealingAdaptiveFloat.cc
  GradientDescentFloat.cc
  LevenbergMarquardtSparseBKFloat.cc
  LineSearchFloat.cc
  ParticleSwarmFloat.cc
  SearchableNumericFloat.cc
  SearchableSparseFloat.cc

  # Clustering
  ../../include/fl/cluster.h
  Agglomerate.cc
  ClusterMethod.cc
  GaussianMixture.cc
  ${maybeGaussianMixtureParallel}
  KMeans.cc
  KMeansTree.cc
  Kohonen.cc
  SVM.cc

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
target_link_libraries (flNumeric
  flNet
  flBase
  ${FL_LIBRARIES}
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flNumeric
    RUNTIME DESTINATION bin64
    LIBRARY DESTINATION lib64
    ARCHIVE DESTINATION lib64
  )
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flNumeric
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

add_executable (testNumeric test.cc)
target_link_libraries (testNumeric
  flNumeric
  flNet
  flBase
  ${FL_LIBRARIES}
)
