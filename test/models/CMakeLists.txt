# This is to get MPIEXEC
find_package(MPI)
# Needed to run custom tests - `test.sh`
find_program(BASH_EXECUTABLE NAMES bash REQUIRED)

include_directories(
  "${ROSS_SOURCE_DIR}"
  "${ROSS_BINARY_DIR}"
  "${DORYTA_SOURCE_DIR}"
  "${DORYTA_BINARY_DIR}"
)

# Helper functions/macros. src: https://stackoverflow.com/a/7788165
macro(subdirlist_macro result curdir)
  file(GLOB children RELATIVE ${curdir} ${curdir}/*)
  set(dirlist "")
  foreach(child ${children})
    if(IS_DIRECTORY ${curdir}/${child})
      list(APPEND dirlist ${child})
    endif()
  endforeach()
  set(${result} ${dirlist})
endmacro()

# Creating executables and defining tests
subdirlist_macro(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

# For each subdirectory inside test/models we create the rules to compiles all
# `.main.c` files and how to test them. By default the main.c executable is
# run, then it's output is inspected is if there is any.
foreach(subdir ${SUBDIRS})
    message(STATUS "Creating testing structure ${subdir}")
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/test.sh")
        set(bash_test "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/test.sh")
    endif()

    file(GLOB bins_subdir
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/
        ${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/test_*.main.c)
    # message(STATUS "These are the files present in test ${bins_subdir}")

    list(LENGTH bins_subdir num_bins_subdir)
    # message(STATUS "Total number of 'to-be' executables ${num_bins_subdir}")
    if(num_bins_subdir EQUAL 0)
        message(SEND_ERROR "No files to compile were found in subdirectory "
                "`${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/`")
    else()
        # test.sh has to exist if there are multiple .main.c
        if(num_bins_subdir GREATER 1 AND NOT bash_test)
            message(SEND_ERROR "There are multiple binaries for subdirectory "
                    "`${subdir}`. Testing procedure `test.sh` is needed.")
        endif()

        # "empty" list where all targets for this directory will be stored
        set(test_targets)

        # Each file has the form `test_*.main.c` where the asterisk is a
        # placeholder for the name of the test. The name of a test CANNOT
        # contain underscores `_` or the code will fail
        #
        # A more general form for the file is `test_*_np=*_end=*.main.c` frow
        # which we can extract the parameters to use to run the model
        foreach(filename ${bins_subdir})
            # Default parameters for simulation if they are not defined in file
            # name
            set(mpi_count 1) # np=1
            set(doryta_end 1) # end=1

            string(REGEX MATCH
                "^test_([^_ \n\t]+)(_np=[0-9]+)?(_end=[0-9.]+)?\.main\.c$"
                filename_match
                ${filename})
            
            if(NOT filename_match)
                message(SEND_ERROR "The file `${filename}` under `${subdir}` "
                   "does not conform to the file naming conventions for tests.")
            endif()

            # Setting parameters for execution of file. They are all ignored,
            # except for `test_name`, if `check.sh` is present.
            set(test_name ${CMAKE_MATCH_1})
            set(match_3 ${CMAKE_MATCH_3}) # saving variable before it's rewritten

            if(CMAKE_MATCH_2) # np=?
                string(REGEX MATCH "=(.*)$" dummy_match_out ${CMAKE_MATCH_2})
                set(mpi_count ${CMAKE_MATCH_1})
            endif()
            if(match_3) # end=?
                string(REGEX MATCH "=(.*)$" dummy_match_out ${match_3})
                set(doryta_end ${CMAKE_MATCH_1})
            endif()

            set(executable_name "${subdir}_${test_name}")
            # message(STATUS "Declaring executable `${executable_name}` for "
            #     "input file `${subdir}/${filename}`")

            add_executable(${executable_name} ${subdir}/${filename})
            target_link_libraries(${executable_name} PRIVATE doryta_lib m ROSS)
            list(APPEND test_targets $<TARGET_FILE:${executable_name}>)
        endforeach()
    endif()
    
    if(bash_test)
        add_test(NAME test-${subdir}
            COMMAND ${BASH_EXECUTABLE} ${bash_test} ${test_targets} 
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
    else()
        add_test(NAME test-${subdir}
            COMMAND ${MPIEXEC} -np ${mpi_count} "../${executable_name}"
                    --end=${doryta_end} --synch=3
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
    endif()

    # TODO: add procedure to compare output!
    # It checks first that `check.sh` exists. If it does, it runs it, otherwise it
    # compares all files against all the others
endforeach(subdir)
