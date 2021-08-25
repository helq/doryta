# Support for C_STANDARD 17 was added in version 3.21
cmake_minimum_required(VERSION 3.21)
project(doryta)
#include(CTest)

set(CMAKE_C_STANDARD 17)
option(CMAKE_C_EXTENSIONS "C extensions used sparcingly only to check for validity of code" OFF)

add_subdirectory(external)

# Setting optimizations (ignoring external as there is nothing we
# can do for them. Their badly written, but our library shouldn't)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=native \
-Wall -Wextra -Wpedantic \
-Wformat=2 -Wno-unused-parameter -Wshadow \
-Wwrite-strings -Wold-style-definition \
-Wnested-externs -Wmissing-include-dirs \
-Werror")
#-Wstrict-prototypes -Wredundant-decls 

add_subdirectory(src)
#add_subdirectory(test)

#enable_testing()
