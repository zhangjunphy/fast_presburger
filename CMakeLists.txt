cmake_minimum_required(VERSION 3.13.4)
project(fast_presburger LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to conform to")

include_directories(${CMAKE_SOURCE_DIR}/include)

add_compile_options(-fno-rtti)

aux_source_directory(src FAST_PRESBURGER_SRC)
add_library(fast-presburger ${FAST_PRESBURGER_SRC})

include_directories(test)
