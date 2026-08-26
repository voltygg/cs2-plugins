# Conan replaces these class attributes at runtime, causing Pyright false positives.
# pyright: reportAttributeAccessIssue=false

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class CS2PluginsConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    # VoltMod supplies cpr, nlohmann_json, libpqxx, HL2SDK, and Metamod transitively.
    requires = ("voltmod/[~1.2]",)

    default_options = {
        "*:shared": False,
        "openssl/*:no_apps": True,
        "openssl/*:no_fips": True,
        # admin-system and anticheat both use the Database module.
        "voltmod/*:with_postgres": True,
    }

    def build_requirements(self):
        self.test_requires("doctest/2.5.2")

    def layout(self):
        # Match the build path used by voltmod and the CMake presets.
        toolchain = "windows-msvc" if self.settings.os == "Windows" else "linux-steamrt"
        preset = f"{toolchain}-{str(self.settings.build_type).lower()}"
        self.folders.build = f"build/{preset}"
        self.folders.generators = f"build/{preset}/generators"

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.user_presets_path = False
        toolchain.variables["CMAKE_POSITION_INDEPENDENT_CODE"] = True
        toolchain.generate()
