# This is to get MPIEXEC_MAX_NUMPROCS
find_package(MPI)
# Needed to run custom tests - `test.sh`
find_program(BASH_EXECUTABLE NAMES bash REQUIRED)

include_directories(
  "${external_includes}"
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

## Default bash scripts
# checks whether two directories contain the same files
set(bash_check_global "${CMAKE_CURRENT_SOURCE_DIR}/check-dirs.sh")
# runs binaries with necessary parameters
set(bash_test_global "${CMAKE_CURRENT_SOURCE_DIR}/test.sh")
# cleans prior executions
set(bash_test_pre "${CMAKE_CURRENT_SOURCE_DIR}/test-pre.sh")
# checks that the net total events in the simulation coincide with given number
set(bash_check_net_events "${CMAKE_CURRENT_SOURCE_DIR}/check-net-events.sh")

# For each subdirectory inside test/models we create the rules to compile all
# `.main.c` files and how to test them. By default the main.c executable is
# run, then it's output is inspected if there is an `expected_output`
# directory.
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
        if(NOT bash_test)
            message(SEND_ERROR "No files to compile were found in subdirectory "
                    "`${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/` nor `test.sh`")
        endif()
        message(STATUS "No files to compile were found in subdirectory "
                "`${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/`. "
                "`test.sh` will be run with the doryta binary as input.")
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
            target_link_libraries(${executable_name} PRIVATE doryta_lib ${external_libraries})
            list(APPEND test_targets $<TARGET_FILE:${executable_name}>)
        endforeach()
    endif()
    
    # Defining test to be run. Either `test.sh` or executing the binary
    # generated by `test_*.main.c`
    if(bash_test)
        # Doryta binary is passed on and executed for the test unless `.main.c`
        # files have been defined in the directory
        if(num_bins_subdir EQUAL 0)
            add_test(NAME test-${subdir}-bin
                COMMAND ${BASH_EXECUTABLE} ${bash_test_pre}
                        ${bash_test}
                        ${MPIEXEC_MAX_NUMPROCS} # $1 = number of mpi ranks "available". Probably, unreliable
                        $<TARGET_FILE:doryta>   # $2 = doryta bin
                        ${DORYTA_MODELS_DIR}    # $3 = doryta models dir
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
        else()
            add_test(NAME test-${subdir}-bin
                COMMAND ${BASH_EXECUTABLE} ${bash_test_pre}
                        ${bash_test}
                        ${MPIEXEC_MAX_NUMPROCS}
                        ${test_targets} 
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
        endif()
    else()
        add_test(NAME test-${subdir}-bin
            COMMAND ${BASH_EXECUTABLE} ${bash_test_pre}
                ${bash_test_global}
                $<TARGET_FILE:${executable_name}>  # $1       = binary
                ${mpi_count}                       # -np $2
                ${doryta_end}                      # --end=$3
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
    endif()

    # Comparing output results if the folder `expected_output` is present
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/expected_output"
       OR EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/check-output.sh")
        # The comparison is performed either by custom or global `check.sh`
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/check-output.sh")
            set(bash_check "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/check-output.sh")
        else()
            set(bash_check "${bash_check_global}")
        endif()

        add_test(NAME test-${subdir}-checking-output
            COMMAND
            ${BASH_EXECUTABLE} -x
            "${bash_check}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/expected_output"
            "${CMAKE_CURRENT_BINARY_DIR}/${subdir}/output"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
        set_tests_properties(test-${subdir}-checking-output
            PROPERTIES DEPENDS test-${subdir}-bin)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/net_events_expected.txt")
        add_test(NAME test-${subdir}-checking-net-events
            COMMAND
            ${BASH_EXECUTABLE} -x
            "${bash_check_net_events}"
            "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${subdir})
        set_tests_properties(test-${subdir}-checking-net-events
            PROPERTIES DEPENDS test-${subdir}-bin)
    endif()
endforeach(subdir)
