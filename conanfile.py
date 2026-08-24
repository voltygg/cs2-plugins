# Conan rebinds class attributes at runtime; ignore the Pyright false positives.
# pyright: reportAttributeAccessIssue=false

import os

from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class CS2PluginsConan(ConanFile):
    name = "cs2-plugins"
    settings = "os", "compiler", "build_type", "arch"
    package_type = "shared-library"

    # cpr, nlohmann_json and libpqxx arrive transitively through cs2-kit, along with
    # the HL2SDK and Metamod packages.
    requires = ("cs2-kit/[~1.2]",)

    default_options = {
        "*:shared": False,
        "openssl/*:no_apps": True,
        "openssl/*:no_fips": True,
        # admin-system and anticheat both use the Database module.
        "cs2-kit/*:with_postgres": True,
    }

    def set_version(self):
        # version.txt, same as CMakeLists.txt reads.
        with open(os.path.join(self.recipe_folder, "version.txt"), encoding="utf-8") as handle:
            self.version = handle.read().strip()

    def build_requirements(self):
        self.test_requires("doctest/2.5.2")

    def layout(self):
        # Where cs2kit-build and the CMake presets expect the toolchain. Declared here rather
        # than passed on the command line, so the recipe is the one description of it.
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
