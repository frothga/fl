SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


VPATH = $(SOURCE_LIB)

libflBase.a: random.o Parameters.o
	ar rcsv $(SOURCE_LIB)/$@ $^

clean:
	rm -f *~ *.o core core.* Dependencies $(SOURCE_LIB)/libflBase.a

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies