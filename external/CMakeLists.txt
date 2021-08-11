# EXCLUDE_ALL is used to prevent all "install" cmake commands from executing.
# By default, running "make install" will copy all header files, the
# statically compiled ROSS libraries and some configuration files. None of
# that is needed for a standalone executable/model.
add_subdirectory(ROSS EXCLUDE_FROM_ALL)
