find_package(GTest REQUIRED)
find_package(nlohmann_json)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE unleash GTest::gtest_main GTest::gmock
                                    nlohmann_json::nlohmann_json)

include(GoogleTest)
gtest_discover_tests(tests)

if(ENABLE_TEST_COVERAGE)
    target_compile_options(unleash PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
    target_link_options(unleash PUBLIC -fprofile-arcs -ftest-coverage)
endif()

file(
  GLOB FILELIST
  LIST_DIRECTORIES true
  ${CMAKE_SOURCE_DIR}/client-specification/specifications/*)
file(COPY ${FILELIST} DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/specification)
