
add_library(
  unleash
  unleashclient.cpp
  feature.cpp
  api/cprclient.cpp
  strategies/strategy.cpp
  strategies/default.cpp
  strategies/userwithid.cpp
  strategies/applicationhostname.cpp
  strategies/flexiblerollout.cpp
  strategies/remoteaddress.cpp
  strategies/murmur3hash.cpp
  strategies/gradualrolloutuserid.cpp
  strategies/gradualrolloutsessionid.cpp
  strategies/gradualrolloutrandom.cpp)
# add_library(unleash::unleash ALIAS unleash)
target_include_directories(
  unleash PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>")
target_link_libraries(unleash PRIVATE cpr::cpr
                                      nlohmann_json::nlohmann_json)
set_target_properties(unleash PROPERTIES VERSION ${unleash_VERSION}
                                         SOVERSION ${unleash_VERSION_MAJOR})

target_compile_features(unleash PUBLIC cxx_std_17)


