vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO aruizs/unleash-client-cpp
    REF "v${VERSION}"
    SHA512 892f15ffbfb773f990edfe5b306d410bfb234fdd7954857720b1b97e703ea38c272c3990929814324a59805669c49fac1c19f952f246f02e675fddec4f32dfa9
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
