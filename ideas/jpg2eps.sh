#!/bin/bash

if [ $1 != "ppm" -a $1 != "pgm" ]; then
    echo "First argument must be either \"ppm\" or \"pgm\"."
    exit 1
fi

if [ -z "$2" -o -z "$3" ]; then
    echo "Syntax: $0 mode input-file output-file"
    exit 1
fi

PNM_FULL=`mktemp /tmp/to_eps_full.XXXXXX`
if [ $? -ne 0 ]; then
    echo "$0: Can't create \"full\" pnm file, exiting..."
    exit 1
fi
PNM_HEAD=`mktemp /tmp/to_eps_head.XXXXXX`
if [ $? -ne 0 ]; then
    echo "$0: Can't create \"head\" pnm file, exiting..."
    exit 1
fi
PNM_BODY=`mktemp /tmp/to_eps_body.XXXXXX`
if [ $? -ne 0 ]; then
    echo "$0: Can't create \"body\" pnm file, exiting..."
    exit 1
fi

echo "converting $2 to $1 format ${PNM_FULL}"
convert -depth 8 $2 $1:${PNM_FULL}

# Use PNM_BODY as temp file
echo "Extracting ppm header"
awk 'BEGIN {tfld = 0}
     {print $0}
     /^[^#]/ {tfld = tfld + NF; if (tfld >= 4) exit;}' \
    ${PNM_FULL} > ${PNM_BODY}
HEAD_LEN=`wc -c < ${PNM_BODY}`
echo "Header length is: ${HEAD_LEN}"
grep '^[^\#]' ${PNM_BODY} > ${PNM_HEAD}

echo "header of ppm is:"
cat ${PNM_HEAD}

if [ $1 = "ppm" -a `head -c 2 ${PNM_HEAD}` != "P6" ]; then
    echo "Error with ppm format. Giving up."
    rm ${PNM_FULL} ${PNM_HEAD} ${PNM_BODY}
    exit 1
fi

if [ $1 = "pgm" -a `head -c 2 ${PNM_HEAD}` != "P5" ]; then
    echo "Error with pgm format. Giving up."
    rm ${PNM_FULL} ${PNM_HEAD} ${PNM_BODY}
    exit 1
fi

WIDTH=`awk 'BEGIN {RS="\n#"} {print $2; exit}' ${PNM_HEAD}`
echo "Image width  is: ${WIDTH}"

HEIGHT=`awk 'BEGIN {RS="\n#"} {print $3; exit}' ${PNM_HEAD}`
echo "Image height is: ${HEIGHT}"

echo "Extracting ppm body"
tail -c +`echo ${HEAD_LEN} + 1 | bc` ${PNM_FULL} > ${PNM_BODY}

echo "First 10 bytes of ppm body are:"
head -c 10 ${PNM_BODY}
echo ""

if [ $1 = "ppm" ]; then
    COLORSPACE="/DeviceRGB"
    DECODE="[0 1 0 1 0 1]"
    COLORS="3"
else
    COLORSPACE="/DeviceGray"
    DECODE="[0 1]"
    COLORS="1"
fi

echo "Outputting PS header"
echo "%!PS

%%BoundingBox: 0 0 ${WIDTH} ${HEIGHT}
/width  ${WIDTH} def /height ${HEIGHT} def

${COLORSPACE} setcolorspace
width height scale
<<
  /ImageType 1
  /Width width /Height height
  /BitsPerComponent 8
  /Decode ${DECODE}
  /ImageMatrix [width 0 0 height neg 0 height]
  /DataSource
    currentfile
    /ASCIIHexDecode filter
    << >> /DCTDecode filter
>>
image" > $3

echo "Converting PNM to PS and appending to header"
echo "%!PS

/infile (${PNM_BODY}) (r) file def

/outfile1 (%stdout) (w) file def
/outfile2 outfile1 /ASCIIHexEncode filter def
/outfile3 outfile2
  << /Columns ${WIDTH} /Rows ${HEIGHT} /Colors ${COLORS} >>
  /DCTEncode filter def

% Now convert
{
    infile read not {exit} if
    outfile3 exch write
} loop

outfile3 closefile
outfile2 closefile
outfile1 closefile
" | gs -dQUIET -  >> $3

echo "removing temp files: ${PNM_FULL} ${PNM_HEAD} ${PNM_BODY}"
rm ${PNM_FULL} ${PNM_HEAD} ${PNM_BODY}
