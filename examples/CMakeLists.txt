add_executable(quickstart quickstart.cpp)
target_link_libraries(quickstart PRIVATE unleash)
target_compile_features(quickstart PRIVATE cxx_std_17)
