vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO aruizs/unleash-client-cpp
    REF "v${VERSION}"
    SHA512 fa8214715d376839147b0b3f8f89d5ea5bac8c824b5814128b21cfabc3d9376cce2962fc373188b151878c3efa067c0f51cb5c66c55ffd25003e73426a5e7555
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DUNLEASH_ENABLE_TESTING=OFF
        -DUNLEASH_BUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME unleash CONFIG_PATH lib/cmake/unleash)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
