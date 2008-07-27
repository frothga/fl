# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# Copyright 2005, 2008 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.


SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config

MODULE = Numeric
VPATH = $(SOURCE_LIB)
LOADLIBS = -L$(SOURCE_LIB) -l$(NS)Base -l$(NS)Net $(LAPACK_LIB)


MATRIX_DOUBLE_O = \
	MatrixDouble.o \
	VectorDouble.o \
	MatrixFixedDouble22.o \
	MatrixFixedDouble33.o \
	MatrixPackedDouble.o \
	MatrixIdentityDouble.o \
	MatrixDiagonalDouble.o \
	MatrixSparseDouble.o

MATRIX_FLOAT_O = \
	MatrixFloat.o \
	VectorFloat.o \
	MatrixFixedFloat22.o \
	MatrixFixedFloat33.o \
	MatrixPackedFloat.o \
	MatrixIdentityFloat.o \
	MatrixDiagonalFloat.o \
	MatrixSparseFloat.o

MATRIX_COMPLEX_DOUBLE_O = \
	MatrixComplexDouble.o \
	VectorComplexDouble.o

MATRIX_BOOL_O = \
	MatrixBool.o \
	VectorBool.o \
	MatrixSparseBool.o

MATRIX_INT_O = \
	MatrixInt.o \
	VectorInt.o \
	MatrixSparseInt.o

LAPACK_DOUBLE_O = \
	dsyev.o \
	dspev.o \
	dsygv.o \
	dgeev.o \
	dgelss.o \
	dgelsd.o \
	dgesvd.o \
	dgetrf.o

LAPACK_FLOAT_O = \
	ssyev.o \
	sspev.o \
	ssygv.o \
	sgeev.o \
	sgelss.o \
	sgelsd.o \
	sgesvd.o \
	sgetrf.o

SEARCH_DOUBLE_O = \
	SearchableNumericDouble.o \
	SearchableSparseDouble.o \
	AnnealingAdaptiveDouble.o \
	LevenbergMarquardtDouble.o \
	LevenbergMarquardtSparseBKDouble.o

SEARCH_FLOAT_O = \
	SearchableNumericFloat.o \
	SearchableSparseFloat.o \
	AnnealingAdaptiveFloat.o \
	LevenbergMarquardtFloat.o \
	LevenbergMarquardtSparseBKFloat.o

ROOTS_O = \
	zroots.o

CLUSTER_O = \
	Metric.o \
	ClusterMethod.o \
	KMeans.o \
	KMeansParallel.o \
	Kohonen.o \
	Agglomerate.o

NEURAL_O = \
	NeuralNetwork.o \
	NeuralNetworkBackprop.o \
	NeuralNetworkVector.o

REDUCE_O = \
	DimensionalityReduction.o \
	PCA.o \
	MDA.o

OBJS = $(MATRIX_DOUBLE_O) $(MATRIX_FLOAT_O) $(MATRIX_BOOL_O) $(MATRIX_INT_O) $(MATRIX_COMPLEX_DOUBLE_O) $(LAPACK_DOUBLE_O) $(LAPACK_FLOAT_O) $(SEARCH_DOUBLE_O) $(SEARCH_FLOAT_O) $(ROOTS_O) $(CLUSTER_O) $(NEURAL_O) $(REDUCE_O)

$(LIBPREF)$(MODULE)$(LIBSUFF): $(OBJS)
	$(LIBCOMMAND)


test: test.cc $(LIBPREF)$(MODULE)$(LIBSUFF)
	g++ -o $@ -L$(SOURCE_LIB) $^ -l$(NS)Base $(CPPFLAGS) $(CXXFLAGS) $(LAPACK_LIB)

#CXXFLAGS = -g -O3 -march=pentiumpro
#test: test.cc MatrixDouble.o LevenbergMarquardt.o VectorDouble.o MatrixInt.o MatrixSparseInt.o SearchableNumeric.o VectorInt.o MatrixSparseDouble.o
#	g++ -o $@ $^ $(CPPFLAGS) $(CXXFLAGS) -L/home/rothgang/i386/lib -llapack -lblas -lminpack -lg2c ../base/random.o -lefence


clean:
	rm -f *~ *.o core core.* $(SOURCE_LIB)/$(LIBPREF)$(MODULE)*
	rm -rf Debug Release
	rm -f test$(EXESUF)

distclean: clean
	rm -f Dependencies

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies
