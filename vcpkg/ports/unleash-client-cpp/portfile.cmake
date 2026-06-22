vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO aruizs/unleash-client-cpp
    REF "v${VERSION}"
    SHA512 96e1b4096b2aa3baba2c767f08f9da0e4172dbda0b11516af615596a80cefe7da372be6a49b8fc5a60ab635238c145a3e692ecfafcff26161671b79e0265c08f
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DCMAKE_REQUIRE_FIND_PACKAGE_cpr=ON
        -DCMAKE_REQUIRE_FIND_PACKAGE_nlohmann_json=ON
        -DUNLEASH_ENABLE_TESTING=OFF
        -DUNLEASH_BUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/unleash" PACKAGE_NAME "unleash")

vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
