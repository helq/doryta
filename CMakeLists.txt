# - Support for C_STANDARD 17 was added in version 3.21
# - Support for saving subgroup matches on regexp string related operations started from 3.9
cmake_minimum_required(VERSION 3.21)
project(doryta)

set(CMAKE_C_STANDARD 17)
option(CMAKE_C_STANDARD_REQUIRED ON)
option(CMAKE_C_EXTENSIONS OFF)
# Compiler extensions are turned off to enforce the use of only (mostly)
# standard C functions and definitions. Code written on standard C can often be
# compiled in more platforms and for longer periods of time

add_subdirectory(external)

# Uncomment the following lines if you find problems with the MPI library. In some
# systems, CMake doesn't include MPI into doryta from the ROSS dependencies.
#find_package(MPI REQUIRED)
#include_directories(SYSTEM ${MPI_C_INCLUDE_PATH})

# Setting optimizations (ignoring "external/" as there is nothing we
# can do for those projects. They're badly written, but our library shouldn't)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=native \
-Wall -Wextra -Wpedantic \
-Wformat=2 -Wno-unused-parameter -Wshadow \
-Wwrite-strings -Wold-style-definition \
-Wnested-externs -Wmissing-include-dirs \
-Werror")
#-Wstrict-prototypes -Wredundant-decls 

add_subdirectory(src)

set(DORYTA_MODELS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/models")

string(COMPARE NOTEQUAL "RELEASE" "${CMAKE_BUILD_TYPE}" not_release)
if(BUILD_TESTING AND not_release)
    include(CTest)
    add_subdirectory(test)
endif()
