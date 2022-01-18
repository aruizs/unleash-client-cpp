include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT DEFINED unleash_INSTALL_CMAKEDIR)
  set(unleash_INSTALL_CMAKEDIR
      "${CMAKE_INSTALL_LIBDIR}/cmake/unleash"
      CACHE STRING "Path to unleash CMake files")
endif()

install(
  TARGETS unleash
  EXPORT unleash_Targets
  RUNTIME COMPONENT unleash_Runtime
  LIBRARY COMPONENT unleash_Runtime NAMELINK_COMPONENT unleash_Development
  ARCHIVE COMPONENT unleash_Development
  INCLUDES
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(
  DIRECTORY "${unleash_SOURCE_DIR}/include/" "${unleash_BINARY_DIR}/include/"
  TYPE INCLUDE
  COMPONENT unleash_Development)

if(BUILD_SHARED_LIBS)
  set(type shared)
else()
  set(type static)
endif()

install(
  EXPORT unleash_Targets
  DESTINATION "${unleash_INSTALL_CMAKEDIR}"
  NAMESPACE unleash::
  FILE unleash-${type}-targets.cmake
  COMPONENT unleash_Development)

write_basic_package_version_file(unleashConfigVersion.cmake
                                 COMPATIBILITY SameMajorVersion)

install(
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/unleashConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/unleashConfigVersion.cmake"
  DESTINATION "${unleash_INSTALL_CMAKEDIR}"
  COMPONENT unleash_Development)

# TODO: add additional CPack variables here
include(CPack)
