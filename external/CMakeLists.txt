# EXCLUDE_ALL is used to prevent all "install" cmake commands from executing.
# By default, running "make install" will copy all header files, the
# statically compiled ROSS libraries and some configuration files. None of
# that is needed for a standalone executable/model.
add_subdirectory(ROSS EXCLUDE_FROM_ALL)

# pcg doesn't come with CMake stuff
set(PCG32_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pcg-c-basic")

include_directories(${PCG32_SOURCE_DIR})
add_library(pcg32_lib
    pcg-c-basic/pcg_basic.c
)

# Setting up global variables for easy access
set(external_includes
        "${ROSS_SOURCE_DIR}"
        "${ROSS_BINARY_DIR}"
        # Uncomment in case of using RIO
        #"${RIO_SOURCE_DIR}" "${RIO_BINARY_DIR}"
        "${PCG32_SOURCE_DIR}"
    PARENT_SCOPE)

set(external_libraries
        ROSS m
        #RIO # uncomment if using RIO
        pcg32_lib
    PARENT_SCOPE)
