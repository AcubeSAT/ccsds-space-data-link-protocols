# CCSDS Data Link

A C++ implementation of the TM and TC CCSDS Space Data Link Protocols, for ground stations and spacecrafts.
For an introduction to CCSDS Data Link, refer to [Space Data Link Protocols—Summary of Concept and Rationale (CCSDS 130.2-G-3)](https://ccsds.org/Pubs/130x2g3.pdf).
More specifically, the list of implemented protocols is:

- [TC Space Data Link Protocol (CCSDS 232.0-B-4)](https://ccsds.org/Pubs/232x0b4e1c1.pdf)
- [TM Space Data Link Protocol (CCSDS 132.0-B-3)](https://ccsds.org/Pubs/132x0b3.pdf)
- [Communications Operations Procedure-1 (CCSDS 232.1-B-2)](https://ccsds.org/Pubs/232x1b2e2c1.pdf)
- [SDLS Protocol (CCSDS 355.0-B-2)](https://ccsds.org/Pubs/355x0b2.pdf)

This implementation is compatible with all CCSDS packets protocols at the time of writing, namely
`Space Packets` (see [Space Packet Protocol CCSDS 133.0-B-2](https://ccsds.org/Pubs/133x0b2e2.pdf))
and `Encapsulation Packets` (see [Encapsulation Packet Protocol CCSDS 133.1-B-3](https://ccsds.org/Pubs/133x1b3e1.pdf)),
but can also accept custom data units. For an up-to-date list of supported packets, refer to
[SANA Approved Packet Version Numbers](https://sanaregistry.org/r/packet_version_number/).

Other relevant information: 

- [SANA Spacecraft Identifier Registry](https://sanaregistry.org/r/spacecraftid/) 
- [CCSDS Spacecraft Identification Field Code Assignment Control Procedures (CCSDS 320.0-M-7 )](https://ccsds.org/Pubs/320x0m7c1.pdf)
- [SDLS Protocol - Summary of Concept and Rationale (CCSDS 350.5-G-2)](https://ccsds.org/Pubs/350x5g2.pdf)

### Project structure
The library relies on the CMake build system and Conan for dependency management. To be a viable 
solution for resource constrained systems, the [Embedded Template Library](https://www.etlcpp.com/),
as well as [Tinycrypt](https://github.com/intel/tinycrypt) for HMAC calculations are used. For unit-testing,
[Catch2](https://github.com/catchorg/Catch2) is utilized. A short description of some of the most important directories is: 

- `ci`: Scripts for continuous integration and doxygen
- `inc/Channels`: Classes that hold the ccsds channel configuration during runtime and buffers for intermediate processing
- `inc/COP1`: Classes that hold COP-1 parameters and methods to implement the state machines
- `inc/DataHandlingFunctions`: Core functions for frame processing
- `inc/DataStructures`: Elementary data structures like queue, and memory pool, as well as Transfer Frames and CLCWs
- `inc/NotificationAndLoggingUtilities`: Definitions for function notifications/errors and frame printing functions
- `inc/Platform`: Platform specific utility functions
- `inc/Services`: A collection of methods that use the data handling functions and constitute the user API 
- `inc/Utilities`: Different utility functions. Most notable are the CRC and HMAC calculation functions (software implementation).
- `test`: Catch2 unit tests
- `test_package`: This is a small project used by conan to test if the library can be linked properly 

### Consuming the library
1. (If you haven't already) create a conan profile for your system:
```
conan profile detect
```
2. (If you haven't already) add the SpaceDot repository to conan:
```
conan remote add spacedot https://artifactory.spacedot.gr/artifactory/api/conan/conan
```

3. In your project's ```conanfile.py```, add:

```python
def requirements(self):
        self.requires("ccsds-data-link-layer/1.0")
```

It is necessary to add a path to a channel configuration file (must be named `CCSDSDataLink.def`), as well as a key configuration
file, if SDLS is used (must be named 'CCSDSKeys.def'). If none is provided, then a default configuration will be used (located under `inc/Platform`).
If the project is not in x86, then a platform definitions directory path (of any name) must be provided, with countdown
timer and mutex implementations. Look under `inc/Platform/x86` for more details. All paths have to be absolute:
```python
default_options = {"ccsds-data-link-layer/*:channel_config": os.path.abspath("path/to/CCSDSDataLink.def")
                   "ccsds-data-link-layer/*:keys_config": os.path.abspath("path/to/CCSDSDataLink.def"),
                   "ccsds-data-link-layer/*:platform_definitions_path": os.path.abspath("path/to/platform")
                   "ccsds-data-link-layer/*:frame_printing_functions": "OFF"
                   }
```

The `frame_printing_functions` flag is used to include diagnostic functions that print the fields 
of frames in a pretty format, at the expense of extra memory consumption.

4. In your `CMakeLists.txt`, add:
```txt
find_package(ccsds-data-link-layer CONFIG REQUIRED)

target_link_libraries(TARGET_NAME PRIVATE space_segment_lib) # for ground station application
# OR
target_link_libraries(TARGET_NAME PRIVATE ground_segment_lib) # for a spacecraft application
```

5. (Optional) For resource constrained systems the CRC and HMAC software implementations can be demanding, so they are
    defined as weak for the user to replace them if need be. Look under `inc/Utilities` for the software
    implementations. Do not worry about those functions if there are no plans to use CRC or HMAC. The Idle data generator
    is also defined as weak.

The user API for the library is contained under `inc/Services`. 

#TODO add examples

### Building tests locally (CLI)
1. Clone the repository and enter the directory:
 ```
 git clone git@gitlab.com:acubesat/comms/software/ccsds-data-link-layer.git
 cd ccsds-data-link-layer
 ```
2. (If you haven't already) create a conan profile for your system:
```
conan profile detect
```
3. (If you haven't already) add the SpaceDot repository to conan:
```
conan remote add spacedot https://artifactory.spacedot.gr/artifactory/api/conan/conan
```
4. Download all dependencies with conan:
```
conan install . --build=missing -s build_type=Debug
```
5. Add flags to CMake and generate Makefiles:
```
cmake -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake -DBUILD_UNIT_TESTS=ON -DBUILD_x86=ON
```

6. Build the project:
```
cmake --build build/Debug
```

7. Run the executable:
```
./build/Debug/catch2_unit_testing
```


### Building tests locally (CLion)
1. Repeat steps 1 to 4 as shown in the CLI section. Step 4 needs to be rerun each
   time the conan recipe changes.
2. Open CLion and ensure that cmake and openocd are detected, from
   `file > settings > Build, Execution, Deployment > Embedded Development`.
   If not, install them.
3. Open the project with CLion. On the top right, you should be seeing the
   following CMake and Run/Debug configurations:

   `Debug - Unit Testing`, `Catch2 Unit Testing`

   If not, you need to install them manually
    * Go to `file > settings > Build, Execution, Deployment > CMake` and create a new profile
      with these settings:
        - Build type: `Debug`
        - Toolchain: `Use Default`
        - Generator: `Unix Makefiles`
        - CMake options: `-G "Unix Makefiles" -B build/Debug -S . -DCMAKE_TOOLCHAIN_FILE=build/Debug/generators/conan_toolchain.cmake -DBUILD_UNIT_TESTS=ON -DBUILD_x86=ON`
        - Build directory: `build/Debug`

    * Ensure the 'Enable Profile' and 'Share' boxes are enabled, press OK.
    * On the top right (right next to the build icon) the is the 'Run/Debug configurations' menu. Go to 'Edit Configurations...' and add a new 'CMake Application',
      where the target and executable are `catch2-unit-testing`. Ensure that the `store as project file` box is selected.

4. Reload CMake Project (this can be done by right clicking inside the file explorer)
5. You can now build/run/debug by clicking the respective icons.

### Use CI (Continuous Integration) Locally

#TODO add ci script instructions

The `test_package` directory contains a minimal project, which is used by conan to confirm that the library can be linked
to a consumer project with no issues. To see this in action, either use: 

```
conan create . --build=missing -s build_type=Debug
```

where testing is triggered in the last stage. Alternatively:

```
conan test test_package ccsds-data-link-layer/1.0 --build=missing
```

### LICENCE
The project is licensed under MIT