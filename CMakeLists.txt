cmake_minimum_required(VERSION 3.13)
  
project(streamd LANGUAGES C)

set(CMAKE_VERBOSE_MAKEFILE "ON")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(x11)
# add_subdirectory(wayland)
