import os
from os.path import join

from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.files import copy
from conan.tools.files import get, chdir, mkdir

class CCSDSDataLinkLayer(ConanFile):
    name = "ccsds-data-link-layer"
    version = "1.0"

    # Optional metadata
    license = "MIT"
    author = "SpaceDot - AcubeSAT, acubesat.comms@spacedot.gr"
    url = "https://gitlab.com/acubesat/comms/software/ccsds-data-link-layer"
    description = "Spacecraft and Ground Station implementations of the CCSDS Data Link Layer"
    topics = ("satellite", "ground station", "embedded")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "platform_definitions_path": ["ANY"],  # User specified directory for platform specific implementations (must be abs path)
        "channel_config" : ["ANY"],            # Channel configuration file (must be abs path)
        "frame_printing_functions" : ["ON", "OFF"] # Include those to have access to functions that print frame fields, at the expense of increased memory consumption
    }
    default_options = {
        "shared": False,
        "fPIC": False,
        "platform_definitions_path" : "",
        "channel_config" : "",
        "frame_printing_functions" : "OFF"
    }

    generators = "CMakeDeps"
    exports_sources = "CMakeLists.txt", "src/*", "inc/*", "lib/*"

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
    def requirements(self):
        self.requires("etl/20.37.2", transitive_headers=True)
        self.requires("logger/1.1", transitive_headers=True)
        if self.settings.arch != "armv7":
            self.test_requires("catch2/3.9.0")

    def layout(self):
        cmake_layout(self)
        self.cpp.source.includedirs = ["inc"]
    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["NO_SYSTEM_INCLUDE"] = True
        tc.cache_variables["BUILD_SPACE_SEGMENT"] = "ON"
        tc.cache_variables["BUILD_GROUND_SEGMENT"] = "ON"
        tc.cache_variables["CHANNEL_CONFIG"] = self.options.channel_config
        tc.cache_variables["FRAME_PRINTING_FUNCTIONS"] = self.options.frame_printing_functions

        if self.settings.arch in ["x86", "x86_64"]:
            tc.variables["BUILD_x86"] = True

        tc.generate()
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
    def package(self):
        src_inc = os.path.join(self.source_folder, "inc")
        dst_inc = os.path.join(self.package_folder, "inc")

        # ---- Copy service headers under inc/services ----
        copy(self, "*.h",   src=os.path.join(src_inc, "Services"),
             dst=os.path.join(dst_inc, "services"), keep_path=True)
        copy(self, "*.hpp", src=os.path.join(src_inc, "Services"),
             dst=os.path.join(dst_inc, "services"), keep_path=True)

        # ---- Copy data handling function headers under inc/dataHandling ----
        copy(self, "*.h",   src=os.path.join(src_inc, "DataHandlingFunctions"),
             dst=os.path.join(dst_inc, "dataHandling"), keep_path=True)
        copy(self, "*.hpp", src=os.path.join(src_inc, "DataHandlingFunctions"),
             dst=os.path.join(dst_inc, "dataHandling"), keep_path=True)

        # ---- Copy headers that should not be normally used by the consumer under inc/internals ----
        for internals_dir in ["CCSDSUtilities", "Channels", "COP1", "DataStructures", "NotificationAndLoggingUtilities", "Utilities"]:
            copy(self, "*.h",   src=os.path.join(src_inc, internals_dir),
                 dst=os.path.join(dst_inc, "internals"), keep_path=True)
            copy(self, "*.hpp", src=os.path.join(src_inc, internals_dir),
                 dst=os.path.join(dst_inc, "internals"), keep_path=True)

        # ---- Headers directly under inc: Some need to be visible to services, others must be hidden in internals ----
        copy(self, "CcsdsDefinitions.hpp",   src=src_inc,
             dst=os.path.join(dst_inc, "services"), keep_path=True)
        copy(self, "ChannelObjects.hpp",   src=src_inc,
             dst=os.path.join(dst_inc, "services"), keep_path=True)

        copy(self, "ChannelGeneration.hpp",   src=src_inc,
             dst=os.path.join(dst_inc, "internals"), keep_path=True)
        copy(self, "etl_profile.h",   src=src_inc,
             dst=os.path.join(dst_inc, "internals"), keep_path=True) # TODO fix this profile, works for x86 only
        copy(self, "SecurityAssociation.hpp",   src=src_inc,
             dst=os.path.join(dst_inc, "internals"), keep_path=True)

        if self.settings.arch in ["x86", "x86_64"]:
            copy(self, "*.h",   src=os.path.join(src_inc, "Platform/x86"),
                 dst=os.path.join(dst_inc, "internals"), keep_path=True)
            copy(self, "*.hpp",   src=os.path.join(src_inc, "Platform/x86"),
                 dst=os.path.join(dst_inc, "internals"), keep_path=True)

        # ---- Copy targets by invoking cmake install command ----
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        if self.settings.arch in ["x86", "x86_64"]:
            logger_req = ["logger::log_x86", "logger::log_common"]
        else:
            logger_req = "logger::log_common"

        comp = self.cpp_info.components["space_segment"]
        comp.set_property("cmake_target_name", "ccsds-data-link-layer::space_segment_lib")
        comp.libs = ["space_segment_lib"]
        comp.includedirs = ["inc", "inc/internals", "inc/services", "inc/dataHandling"]
        comp.requires = ["etl::etl"] + logger_req

        comp = self.cpp_info.components["ground_segment"]
        comp.set_property("cmake_target_name", "ccsds-data-link-layer::ground_segment_lib")
        comp.libs = ["ground_segment_lib"]
        comp.includedirs = ["inc", "inc/internals", "inc/services", "inc/dataHandling"]
        comp.requires = ["etl::etl"] + logger_req
