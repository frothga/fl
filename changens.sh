#!/bin/bash

for FILENAME in Makefile *.cc *.h *.tcc; do
  echo $FILENAME
  sed -f $SOURCE_DIR/changens.sed < $FILENAME > changens.temp
  mv changens.temp $FILENAME
done

exit 0
