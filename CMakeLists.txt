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
include_directories(${OPENGL_INCLUDE_DIR})
link_directories(${OPENGL_gl_LIBRARY})
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
include_directories(${GLFW_INCLUDE_DIRS})
link_directories(${GLFW_LIBRARY_DIRS})
if(NOT GLFW_FOUND)
    message(ERROR " GLFW3 not found!")
endif(NOT GLFW_FOUND)

# FIND GLEW
find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()

# EXECUTABLE
ADD_EXECUTABLE(healpixviewer src/healpixviewer.cpp)
target_link_libraries(healpixviewer ${OPENGL_gl_LIBRARY} ${HEALPIX_LIBRARIES} ${GLFW_LIBRARIES})