set(boost_depends "Boost|filesystem")

mitk_create_module(
  DEPENDS PUBLIC MitkCore
  PACKAGE_DEPENDS PRIVATE Poco ${boost_depends}
  # PACKAGE_DEPENDS PRIVATE GuidelinesSupportLibrary
)

# add_subdirectory(cmdapps)


if(BUILD_TESTING)
add_subdirectory(testing)
endif()