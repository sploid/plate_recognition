Automatic number plate recognition library
=================
Cross-platform ANPR open source library ( C++ )

[Measuring recognition](Measurements.md)

Support only Russian plate GOST 50577-93.

Support platforms
-------
* Windows - norm
* Android - norm
* iOS - not tested yet, but should support
* Mac - not tested yet, but should support
* Linux - not tested yet, but should support

Compilers
-------
* Visual Studio 2012 - norm
* Visual Studio 2013 - norm
* mingw-g++ - norm
* Android NDK r9b - norm

Dependencies
-------
* OpenCV for image processing
* Qt for interaction with the OS

Recognition library depends only on OpenCV.

Build with qmake
-------
1. In file "opencv.pri" set path to OpenCV.
2. In file "opencv.pri" set version of OpenCV.
3. In file "auto_test_desktop/main.cpp" hardcoded folder for test images.
