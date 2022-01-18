cmake_minimum_required(VERSION 3.19)

set(unleash_known_comps static shared)
set(unleash_comp_static NO)
set(unleash_comp_shared NO)
foreach(unleash_comp IN LISTS ${CMAKE_FIND_PACKAGE_NAME}_FIND_COMPONENTS)
  if(unleash_comp IN_LIST unleash_known_comps)
    set(unleash_comp_${unleash_comp} YES)
  else()
    set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
        "unleash does not recognize component `${unleash_comp}`.")
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
    return()
  endif()
endforeach()

if(unleash_comp_static AND unleash_comp_shared)
  set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
      "unleash `static` and `shared` components are mutually exclusive.")
  set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
  return()
endif()

set(unleash_static_targets
    "${CMAKE_CURRENT_LIST_DIR}/unleash-static-targets.cmake")
set(unleash_shared_targets
    "${CMAKE_CURRENT_LIST_DIR}/unleash-shared-targets.cmake")

macro(unleash_load_targets type)
  if(NOT EXISTS "${unleash_${type}_targets}")
    set(${CMAKE_FIND_PACKAGE_NAME}_NOT_FOUND_MESSAGE
        "unleash `${type}` libraries were requested but not found.")
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND FALSE)
    return()
  endif()
  include("${unleash_${type}_targets}")
endmacro()

if(unleash_comp_static)
  unleash_load_targets(static)
elseif(unleash_comp_shared)
  unleash_load_targets(shared)
elseif(DEFINED unleash_SHARED_LIBS AND unleash_SHARED_LIBS)
  unleash_load_targets(shared)
elseif(DEFINED unleash_SHARED_LIBS AND NOT unleash_SHARED_LIBS)
  unleash_load_targets(static)
elseif(BUILD_SHARED_LIBS)
  if(EXISTS "${unleash_shared_targets}")
    unleash_load_targets(shared)
  else()
    unleash_load_targets(static)
  endif()
else()
  if(EXISTS "${unleash_static_targets}")
    unleash_load_targets(static)
  else()
    unleash_load_targets(shared)
  endif()
endif()

include(CMakeFindDependencyMacro)

find_dependency(cpr REQUIRED)
find_dependency(nlohmann_json REQUIRED)
