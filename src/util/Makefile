SOURCE_DIR := ../..
-include $(SOURCE_DIR)/Config


VPATH = $(SOURCE_BIN)

PROGRAMS := $(patsubst %.cc,%,$(wildcard *.cc))

default: all
all: $(PROGRAMS)

%: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@

crypt: crypt.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ -lcrypt

invert: invert.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $^ -o $(SOURCE_BIN)/$@ $(LAPACK_LIB)


clean:
	rm -f *~ *.o core core.* Dependencies $(patsubst %,$(SOURCE_BIN)/%,$(PROGRAMS))

depend:
	rm -f Dependencies
	for SOURCEFILE in `ls *.cc`; do \
	  echo $$SOURCEFILE; \
	  $(CXX) -MM $(CPPFLAGS) $$SOURCEFILE >> Dependencies; \
	done

-include Dependencies