macro(run_conan)
  # Download automatically, you can also just copy the conan.cmake file
  if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    message(
      STATUS
        "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
    file(
      DOWNLOAD
      "https://raw.githubusercontent.com/conan-io/cmake-conan/v0.16.1/conan.cmake"
      "${CMAKE_BINARY_DIR}/conan.cmake"
      EXPECTED_HASH
        SHA256=396e16d0f5eabdc6a14afddbcfff62a54a7ee75c6da23f32f7a31bc85db23484
      TLS_VERIFY ON)
  endif()

  set(ENV{CONAN_REVISIONS_ENABLED} 1)
  list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
  list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

  include(${CMAKE_BINARY_DIR}/conan.cmake)

  # For multi configuration generators, like VS and XCode
  if(NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Single configuration build!")
    set(LIST_OF_BUILD_TYPES ${CMAKE_BUILD_TYPE})
  else()
    message(STATUS "Multi-configuration build: '${CMAKE_CONFIGURATION_TYPES}'!")
    set(LIST_OF_BUILD_TYPES ${CMAKE_CONFIGURATION_TYPES})
  endif()

  foreach(TYPE ${LIST_OF_BUILD_TYPES})
    message(STATUS "Running Conan for build type '${TYPE}'")

    # Detects current build settings to pass into conan
    conan_cmake_autodetect(settings BUILD_TYPE ${TYPE})

    # PATH_OR_REFERENCE ${CMAKE_SOURCE_DIR} is used to tell conan to process the
    # external "conanfile.py" provided with the project Alternatively a
    # conanfile.txt could be used
    if(NOT CONAN_EXPORTED) # if not conan in local cache
      conan_cmake_install(
        PATH_OR_REFERENCE
        ${CMAKE_SOURCE_DIR}
        BUILD
        missing
        # Pass compile-time configured options into conan
        OPTIONS
        SETTINGS
        ${settings})
    endif()
  endforeach()

endmacro()
