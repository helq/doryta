# - Support for C_STANDARD 17 was added in version 3.21
# - Support for saving subgroup matches on regexp string related operations started from 3.9
cmake_minimum_required(VERSION 3.21)
project(doryta)

set(CMAKE_C_STANDARD 17)
option(CMAKE_C_EXTENSIONS OFF)
option(CMAKE_C_STANDARD_REQUIRED ON)

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

string(COMPARE NOTEQUAL "RELEASE" "${CMAKE_BUILD_TYPE}" not_release)
if(BUILD_TESTING AND not_release)
    include(CTest)
    add_subdirectory(test)
endif()
