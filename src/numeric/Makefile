SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


VPATH = $(SOURCE_LIB)


MATRIX_DOUBLE_O = \
	MatrixDouble.o \
	VectorDouble.o \
	Matrix2x2Double.o \
	Matrix3x3Double.o \
	MatrixPackedDouble.o \
	MatrixIdentityDouble.o \
	MatrixDiagonalDouble.o \
	MatrixSparseDouble.o

MATRIX_FLOAT_O = \
	MatrixFloat.o \
	VectorFloat.o \
	Matrix2x2Float.o \
	Matrix3x3Float.o \
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

SEARCH_O = \
	SearchableNumeric.o \
	SearchableSparse.o \
	AnnealingAdaptive.o \
	LevenbergMarquardt.o \
	LevenbergMarquardtSparseBK.o

ROOTS_O = \
	zroots.o

CLUSTER_O = \
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

libflNumeric.a: $(MATRIX_DOUBLE_O) $(MATRIX_FLOAT_O) $(MATRIX_BOOL_O) $(MATRIX_INT_O) $(MATRIX_COMPLEX_DOUBLE_O) $(SEARCH_O) $(ROOTS_O) $(CLUSTER_O) $(NEURAL_O) $(REDUCE_O)
	ar rcsv $(SOURCE_LIB)/$@ $^


test: test.cc libflNumeric.a
	g++ -o $@ $^ $(CPPFLAGS) $(CXXFLAGS) -L/home/rothgang/i386/lib -llapack -lblas -lg2c ../base/random.o

#CXXFLAGS = -g -O3 -march=pentiumpro
#test: test.cc MatrixDouble.o LevenbergMarquardt.o VectorDouble.o MatrixInt.o MatrixSparseInt.o SearchableNumeric.o VectorInt.o MatrixSparseDouble.o
#	g++ -o $@ $^ $(CPPFLAGS) $(CXXFLAGS) -L/home/rothgang/i386/lib -llapack -lblas -lminpack -lg2c ../base/random.o -lefence


clean:
	rm -f *~ *.o core core.* test $(SOURCE_LIB)/libflNumeric.a

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies