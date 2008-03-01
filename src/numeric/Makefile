# Author: Fred Rothganger
# Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
#                         Univ. of Illinois.  All rights reserved.
# Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
# for details.
#
#
# Revisions 1.6, 1.8 thru 1.11 Copyright 2005 Sandia Corporation.
# Revisions 1.13 thru 1.15     Copyright 2008 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Lesser General Public License.  See the file
# LICENSE for details.
#
#
#------------------------------------------------------------------------------
# $Log$
# Revision 1.15  2007/03/23 12:17:03  Fred
# Use CVS Log to generate revision history.
#
# Revision 1.14  2006/02/15 14:13:02  Fred
# Delete test program using $(EXESUF).
# 
# Revision 1.13  2006/02/03 14:32:06  Fred
# Change build system to handle shared libraries as well as static archives.
# 
# Add distclean rule.  They key difference is that now clean won't remove the
# Dependencies file.
# 
# Break the dependency on the image library (a circular dependency), allowing
# shared libraries to work.  Another consequency is that we don't depend on
# freetype2 any more.
# 
# Revision 1.12  2005/10/13 04:14:25  Fred
# Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.
# 
# Revision 1.11  2005/10/13 03:35:57  Fred
# Add Sandia distribution terms.
# 
# Add dgelsd and sgelsd
# 
# Revision 1.10  2005/09/12 03:46:27  Fred
# Rename ?getri.cc to ?getrf.cc, since that is the LAPACK call shared by all
# the functions in those files.
# 
# Revision 1.9  2005/09/11 15:23:59  Fred
# Change lapack*.h into single set of templates in lapack.h.  Move
# implementations to separate files.
# 
# Revision 1.8  2005/09/10 13:40:52  Fred
# Clarify revision history.  Add Sandia copyright.  This will probably need to
# be changed again before final release to add license information.
# 
# Add reference to freetype2 (temporary hack, hopefully).
# 
# Make proper reference to lapack library, rather than explicit path.
# 
# Revision 1.7  2005/04/23 19:37:23  Fred
# Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
# 11/21.
# 
# Revision 1.6  2005/01/22 21:00:25  Fred
# MSVC compilability fix:  Clean intermediate files.
# 
# Revision 1.5  2004/09/08 17:14:58  rothgang
# Remove Dependencies for clean.  (Dependencies are not really needed for a
# clean build, because all .o files are missing.  Not having Dependencies makes
# distribution easier for user.)
# 
# Revision 1.4  2004/07/22 15:20:23  rothgang
# Make float type specific versions of search classes.
# 
# Revision 1.3  2004/04/19 18:33:05  rothgang
# Add agglomerative clustering.  Rename PCA and add MDA.
# 
# Revision 1.2  2004/04/06 21:16:45  rothgang
# Add neural network, dimensionality reduction, clustering, and root finding.
# 
# Revision 1.1  2003/07/08 23:19:47  rothgang
# branches:  1.1.1;
# Initial revision
# 
# Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
# Imported sources
#------------------------------------------------------------------------------


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
