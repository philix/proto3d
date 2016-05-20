cmake_minimum_required(VERSION 2.8.8)
project(proto3d)

# Passing -fno-exceptions to the compiler is not possible yet due to some
# reliance on STL.
OPTION(PROTO3D_EXCEPTIONS
  "Enable code that throws exceptions from proto3d" ON)
OPTION(PROTO3D_GLEW
  "Include GLEW headers after OpenGL headers" ON)
OPTION(PROTO3D_GLM
  "Enable code that relies on GLM" OFF)
OPTION(PROTO3D_STB_IMAGE
  "Compile stb_image.c and allow code using it" ON)

# C++ 11
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11
  support. Please use a different C++ compiler.")
endif()

# Maximum warnings level minus some
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Wno-unused-parameter -Wno-writable-strings")

# Extra flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")

# Add include/ to the include path
include_directories(${PROJECT_SOURCE_DIR})

# Add .c[pp] files to the static library
set(PROTO3D_SOURCE_FILES proto3d.cpp stb_image.c)
if(PROTO3D_STB_IMAGE)
  add_definitions(-DPROTO3D_USE_STB)
endif(PROTO3D_STB_IMAGE)
add_library(proto3d STATIC ${PROTO3D_SOURCE_FILES})

if(PROTO3D_EXCEPTIONS)
  add_definitions(-DPROTO3D_USE_EXCEPTIONS)
endif()

# Use pkg-config to find some libraries
include(FindPkgConfig)

# Find OpenGL
include(FindOpenGL)
if(OPENGL_FOUND)
  target_link_libraries(proto3d ${OPENGL_LIBRARIES})
else()
  message(FATAL_ERROR "OpenGL not found.")
endif()

# Find GLEW
if(PROTO3D_GLEW)
  include(FindGLEW)
  if(GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    target_link_libraries(proto3d ${GLEW_LIBRARIES})
  else()
    message(FATAL_ERROR "GLEW not found. Install it or pass -DPROTO3D_GLEW=OFF to disable it.")
  endif()
  add_definitions(-DPROTO3D_USE_GLEW)
endif()

# Include glm
if(PROTO3D_GLM)
  # GLM is header-only
  include_directories(deps/glm)
  add_definitions(-DPROTO3D_USE_GLM)
endif(PROTO3D_GLM)
