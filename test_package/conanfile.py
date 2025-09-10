from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.build import can_run
import os

class TestPackageConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    default_options = {"ccsds-data-link-layer/*:channel_config": os.path.abspath("CCSDSDataLink.def"),
                       "ccsds-data-link-layer/*:keys_config":    os.path.abspath("CCSDSKeys.def")}

    def requirements(self):
        # Use the package under test
        self.requires(self.tested_reference_str)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        tc = CMakeToolchain(self)
        tc.generate()

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            # Example test for space_segment
            bin_path_space = os.path.join(self.cpp.build.bindir, "test_space")
            self.run(bin_path_space, env="conanrun")

            # Example test for ground_segment
            bin_path_ground = os.path.join(self.cpp.build.bindir, "test_ground")
            self.run(bin_path_ground, env="conanrun")