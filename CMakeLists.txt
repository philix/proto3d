cmake_minimum_required(VERSION 3.1)   # First version with CMAKE_CXX_STANDARD.
project(proto3d_demos)

# Maximum warnings level minus some
set(CMAKE_CXX_FLAGS
  "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -Wno-unused-parameter -Wno-writable-strings")

# Extra flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_EXTRA_FLAGS}")

include(Proto3D.cmake)

# Include demos
add_subdirectory(demos/events_and_shader)
