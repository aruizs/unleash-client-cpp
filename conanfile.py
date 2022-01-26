from conans import ConanFile
from conans import ConanFile, CMake, tools

required_conan_version = ">=1.33.0"


class UnleashConan(ConanFile):
    name = "unleash-client-cpp"
    version = "0.0.1"
    homepage = "https://github.com/aruizs/unleash-client-cpp/"
    license = "MIT"
    url = "https://github.com/conan-io/conan-center-index"
    description = "Unleash Client SDK for C++ projects."
    topics = ("unleash", "feature", "flag", "toggle")
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {"shared": False}
    generators = "cmake_find_package"
    exports_sources = "CMakeLists.txt", "src/*", "include/*", "cmake/*", "packaging/*", "test/*", "client-specification/*"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }
    requires = (
        "gtest/cci.20210126",
        "cpr/1.7.2",
        "nlohmann_json/3.10.5",
    )

    short_paths = True

    _cmake = None

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            del self.options.fPIC

    def _configure_cmake(self):
        if self._cmake:
            return self._cmake
        self._cmake = CMake(self)
        self._cmake.definitions["BUILD_SHARED_LIBS"] = self.options.shared
        self._cmake.definitions["ENABLE_TESTING"] = False
        self._cmake.definitions["ENABLE_TEST_COVERAGE"] = False
        self._cmake.configure()
        return self._cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()

    def package(self):
        self.copy("LICENSE", dst="licenses")
        cmake = self._configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.libs = ["unleash"]
        self.cpp_info.names["cmake_find_package"] = "unleash"
