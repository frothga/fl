# Author: Fred Rothganger
# Copyright 2009, 2010 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
# Distributed under the GNU Leser General Public License.  See the file
# LICENSE for details.


if (Freetype_FOUND)
  include_directories (${FREETYPE_INCLUDE_DIRS})
endif (Freetype_FOUND)

if (TIFF_FOUND)
  set (flImageTIFF ImageFileFormatTIFF.cc)
endif (TIFF_FOUND)

if (JPEG_FOUND)
  set (flImageJPEG ImageFileFormatJPEG.cc)
endif (JPEG_FOUND)

if (PNG_FOUND)
  set (flImagePNG ImageFileFormatPNG.cc)
endif (PNG_FOUND)

if (FFMPEG_FOUND)
  # Should rearrange code so that Video.cc and video.h can be built without FFMPEG present.
  set (flImageFFMPEG
    ../../include/fl/video.h
    Video.cc
    VideoFileFormatFFMPEG.cc
  )
endif (FFMPEG_FOUND)

if (X11_FOUND  OR  MSVC)
  set (flImageX
    flX
    ${X11_LIBRARIES}
    ${Pthread_LIB}
  )
endif (X11_FOUND  OR  MSVC)

add_library (flImage
  # Base Image
  ../../include/fl/image.h
  ../../include/fl/point.h
  Image.cc
  Pixel.cc
  PixelFormat.cc
  PixelBuffer.cc
  Point.cc
  ImageCache.cc

  # Image file formats
  ImageFileFormat.cc
  ImageFileFormatBMP.cc
  ImageFileFormatPGM.cc
  ImageFileFormatEPS.cc
  ImageFileFormatMatlab.cc
  ImageFileFormatNITF.cc
  ${flImageTIFF}
  ${flImageJPEG}
  ${flImagePNG}

  # Canvas
  ../../include/fl/canvas.h
  ../../include/fl/color.h
  Canvas.cc
  CanvasImage.cc
  CanvasPS.cc

  # Filters and convolutions
  ../../include/fl/convolve.h
  Convolution2D.cc
  Gaussian2D.cc
  DifferenceOfGaussians.cc
  GaussianDerivativeFirst.cc
  GaussianDerivativeSecond.cc
  GaussianDerivativeThird.cc
  Laplacian.cc
  Convolution1D.cc
  Gaussian1D.cc
  GaussianDerivative1D.cc
  GaussianDerivativeSecond1D.cc
  FilterHarris.cc
  FilterHarrisEigen.cc
  FilterHessian.cc
  FiniteDifference.cc
  NonMaxSuppress.cc
  IntensityAverage.cc
  IntensityDeviation.cc
  IntensityHistogram.cc
  Normalize.cc
  AbsoluteValue.cc
  Rescale.cc
  Rotate90.cc
  Rotate180.cc
  Transform.cc
  TransformGauss.cc
  Zoom.cc
  ClearAlpha.cc

  # Descriptors
  ../../include/fl/descriptor.h
  Comparison.cc
  Descriptor.cc
  DescriptorCombo.cc
  DescriptorScale.cc
  DescriptorOrientation.cc
  DescriptorOrientationHistogram.cc
  DescriptorContrast.cc
  DescriptorFilters.cc
  DescriptorFiltersTexton.cc
  DescriptorPatch.cc
  DescriptorSchmidScale.cc
  DescriptorSchmid.cc
  DescriptorSpin.cc
  DescriptorSIFT.cc
  DescriptorColorHistogram2D.cc
  DescriptorColorHistogram3D.cc
  DescriptorTextonScale.cc
  DescriptorLBP.cc

  # Interest operators
  ../../include/fl/interest.h
  Interest.cc
  InterestHarris.cc
  InterestHarrisLaplacian.cc
  InterestLaplacian.cc
  InterestHessian.cc
  InterestDOG.cc
  InterestMSER.cc

  # Video
  ../../include/fl/track.h
  KLT.cc
  ${flImageFFMPEG}
)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flImage
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib64
    ARCHIVE DESTINATION lib64
  )
else (CMAKE_SIZEOF_VOID_P EQUAL 8)
  install (TARGETS flImage
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif (CMAKE_SIZEOF_VOID_P EQUAL 8)

add_executable (testImage test.cc)
target_link_libraries (testImage
  ${flImageX}
  flImage
  flNumeric
  flBase
  ${FREETYPE_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${FFMPEG_LIBRARIES}
  ${LAPACK_LIBRARIES}
  ${BLAS_LIBRARIES}
  ${GCC_LIBRARIES}
  ${SOCKET_LIBRARIES}
  ${RT_LIBRARIES}
)
