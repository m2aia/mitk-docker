list(APPEND MITK_USE_Boost_LIBRARIES filesystem)
list(REMOVE_DUPLICATES MITK_USE_Boost_LIBRARIES)
set(MITK_USE_Boost_LIBRARIES ${MITK_USE_Boost_LIBRARIES} CACHE STRING "A semi-colon separated list of required Boost libraries" FORCE)

option(MITK_BUILD_DOCKER "Build docker images." ON)
find_program(DOCKER docker)