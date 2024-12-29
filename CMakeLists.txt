cmake_minimum_required(VERSION 3.12)
project(hdrcpp)

add_library(hdrcpp_histogram INTERFACE)
target_include_directories(hdrcpp_histogram INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(hdrcpp_histogram INTERFACE cxx_std_17)

add_subdirectory(test)