# This is to allow tests to include header files without ugly "../../.."
set(DORYTA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)
set(DORYTA_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}" PARENT_SCOPE)

# Setting up external libraries (includes ROSS)
include_directories("${external_includes}")

# Git stuff is used in configuration header file
list(APPEND CMAKE_MODULE_PATH "${ROSS_SOURCE_DIR}/cmake/")
include(GetGitRevisionDescription)
get_git_head_revision(GIT_RESPEC_ROSS_MODEL GIT_SHA1_ROSS_MODEL)
string(SUBSTRING ${GIT_SHA1_ROSS_MODEL} 0 7 GIT_SHORT_SHA1)

# Building configuration header file
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/doryta_config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/doryta_config.h
  @ONLY
)
set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/doryta_config.h
  PROPERTIES GENERATED TRUE
)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(doryta_lib
  driver/neuron.c
  message.c
  model-loaders/params.c
  model-loaders/strategy/modes.c
  model-loaders/hardcoded/neurons/five_neurons.c
  model-loaders/hardcoded/neurons/gameoflife.c
  model-loaders/hardcoded/spikes/random_spikes.c
  model-loaders/io/load_neurons.c
  model-loaders/io/load_spikes.c
  model-loaders/strategy/layouts/master.c
  model-loaders/strategy/layouts/master.c
  model-loaders/strategy/layouts/standard_layouts.c
  neurons/lif.c
  neurons/lif_beta.c
  probes/firing.c
  probes/lif/voltage.c
  probes/lif_beta/voltage.c
  probes/stats.c
  utils/io.c
  utils/io_load.c
  utils/math.c
  utils/pcg32_random.c
)

# Compiling ROSS doryta model
add_executable(doryta doryta.main.c)
target_link_libraries(doryta
  PRIVATE
    doryta_lib
    ${external_libraries}
)

# Including variables that indicate where the usr/bin is.
# It defines ${CMAKE_INSTALL_FULL_BINDIR}
include(GNUInstallDirs)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/doryta
  DESTINATION ${CMAKE_INSTALL_FULL_BINDIR}
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE
)
