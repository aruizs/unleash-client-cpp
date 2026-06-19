cmake_minimum_required(VERSION 3.14)
project(unleash VERSION 1.0.0)

# C++ language configuration boilerplate
if(NOT DEFINED CMAKE_CXX_VISIBILITY_PRESET AND NOT DEFINED
                                               CMAKE_VISIBILITY_INLINES_HIDDEN)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN YES)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING "Choose the type of build, options are: Debug Release
RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

option(UNLEASH_ENABLE_TESTING "Enable Test Builds" ON)
option(ENABLE_TEST_COVERAGE "Enable Test Coverage" OFF)
option(UNLEASH_BUILD_EXAMPLES "Build the example programs" OFF)
set(UNLEASH_SANITIZER
    ""
    CACHE STRING "Build with a sanitizer: address, thread, or undefined")

if(UNLEASH_SANITIZER)
  message(STATUS "Building with sanitizer: ${UNLEASH_SANITIZER}")
  add_compile_options(-fsanitize=${UNLEASH_SANITIZER} -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=${UNLEASH_SANITIZER})
endif()

find_package(cpr)
find_package(nlohmann_json)

# Let unleash_SHARED_LIBS override BUILD_SHARED_LIBS

if(DEFINED unleash_SHARED_LIBS)
  set(BUILD_SHARED_LIBS "${unleash_SHARED_LIBS}")
endif()

# Create the main unleash library target
add_subdirectory(src)

if(UNLEASH_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

if(UNLEASH_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Generate the export header for unleash and attach it to the target

include(GenerateExportHeader)
generate_export_header(unleash EXPORT_FILE_NAME include/unleash/export.h)
target_compile_definitions(
  unleash PUBLIC "$<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:unleash_STATIC_DEFINE>")
target_include_directories(
  unleash PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>")

# Include the install rules if the user wanted them (included by default when
# top-level)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
               is_top_level)
option(unleash_INCLUDE_PACKAGING "Include packaging rules for unleash" ON)
if(unleash_INCLUDE_PACKAGING)
  add_subdirectory(packaging)
endif()
