HEALPix Viewer
==============

Render a HEALPix data set in false color using modern OpenGL techniques. 

Requirements
------------

Building requires:

* GLFW <http://www.glfw.org>
* GLM <http://glm.g-truc.net>
* CHealpix <http://healpix.sourceforge.net>
* CFitsio <http://heasarc.gsfc.nasa.gov/fitsio/fitsio.html>
* a C++ compiler, `make`, and `pkgconfig`

If, you are on a Mac with Homebrew (Recommended) (http://brew.sh), you can get the dependencies with: 

	brew install homebrew/versions/glfw3
	brew install healpix cfitsio glm pkgconfig glew

To build
--------

Create a new directory:

	mkdir build

Move to the directory:

	cd build

Run Cmake:


	cmake ..
	
Usage
--------
TODO