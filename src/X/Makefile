SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


CPPFLAGS += $(X_INC)
VPATH = $(SOURCE_LIB)

libflX.a: X.o glx.o SlideShow.o GLShow.o
	ar rcsv $(SOURCE_LIB)/$@ $^


test.o: test.cc
	g++ -c test.cc -g $(CPPFLAGS)

test: test.o X.o glx.o GLShow.o SlideShow.o
	g++ -o $@ $^ -L../../lib -lflImage -lflNumeric $(X_LIB) -lpthread -lefence


clean:
	rm -f *~ *.o core core.* test Dependencies $(SOURCE_LIB)/libflX.a

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies