SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


VPATH = $(SOURCE_LIB)

libflNet.a: socket.o
	ar rcsv $(SOURCE_LIB)/$@ $^


test: test.o socket.o
	g++ -o $@ $^


clean:
	rm -f *~ *.o core core.* test Dependencies $(SOURCE_LIB)/libflNet.a
	rm -rf Debug Release

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies