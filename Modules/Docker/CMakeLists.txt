mitk_create_module(
  DEPENDS PUBLIC MitkCore
  PACKAGE_DEPENDS PRIVATE Poco
  # PACKAGE_DEPENDS PRIVATE GuidelinesSupportLibrary
)

# add_subdirectory(cmdapps)


if(BUILD_TESTING)
add_subdirectory(testing)
endif()