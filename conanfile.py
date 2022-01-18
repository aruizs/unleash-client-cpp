from conans import ConanFile


class UnleashProject(ConanFile):
    options = {
    }
    name = "unleash-client-cpp"
    version = "0.1"
    requires = (
        "gtest/cci.20210126",
        "cpr/1.7.2",
        "nlohmann_json/3.10.5",
    )
    generators = "cmake_find_package"

    def requirements(self):
        pass
