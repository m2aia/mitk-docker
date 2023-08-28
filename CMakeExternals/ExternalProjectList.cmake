list(APPEND MITK_USE_Boost_LIBRARIES filesystem)
list(REMOVE_DUPLICATES MITK_USE_Boost_LIBRARIES)
set(MITK_USE_Boost_LIBRARIES ${MITK_USE_Boost_LIBRARIES} CACHE STRING "A semi-colon separated list of required Boost libraries" FORCE)

find_program(DOCKER_EXECUTABLE docker)
