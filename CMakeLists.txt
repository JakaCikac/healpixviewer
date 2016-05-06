cmake_minimum_required (VERSION 3.5)

project (HealpixViewer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

find_package(PkgConfig REQUIRED)
if(NOT PKGCONFIG_FOUND)
    message(ERROR " PKGCONFIG not found!")
endif(NOT PKGCONFIG_FOUND)

# FIND OPENGL
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)

# FIND CHEALPIX
pkg_search_module(HEALPIX REQUIRED chealpix)
include_directories(${HEALPIX_INCLUDE_DIRS})
link_directories(${HEALPIX_LIBRARY_DIRS})
if(NOT HEALPIX_FOUND)
    message(ERROR " HEALPIX not found!")
endif(NOT HEALPIX_FOUND)

# FIND GLFW3
pkg_search_module(GLFW REQUIRED glfw3)
message("glfw: ${GLFW_CFLAGS_OTHER}")
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})
if(NOT GLFW_FOUND)
    message(ERROR " GLFW3 not found!")
endif(NOT GLFW_FOUND)

# EXECUTABLE
SET(SOURCE_FILES src/healpixviewer.cpp)
ADD_EXECUTABLE(healpixviewer ${SOURCE_FILES})