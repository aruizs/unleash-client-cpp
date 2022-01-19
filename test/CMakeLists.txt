find_package(GTest REQUIRED)
find_package(nlohmann_json)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE unleash GTest::gtest_main GTest::gmock
                                    nlohmann_json::nlohmann_json)

include(GoogleTest)
gtest_discover_tests(tests)
