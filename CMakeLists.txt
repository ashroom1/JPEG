cmake_minimum_required(VERSION 3.15)
project(JPEGinCPlusPlus)

set(CMAKE_CXX_STANDARD 14)

add_executable(JPEGinCPlusPlus Decoder.cpp JPEG.h)