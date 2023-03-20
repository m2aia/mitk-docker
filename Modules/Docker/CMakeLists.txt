set(boost_depends "Boost|filesystem")


mitk_create_module(
  DEPENDS PUBLIC MitkCore
  PACKAGE_DEPENDS PUBLIC Poco ${boost_depends}
)

# add_subdirectory(cmdapps)


if(BUILD_TESTING)
add_subdirectory(testing)
endif()