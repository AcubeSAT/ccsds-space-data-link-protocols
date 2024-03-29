cmake_minimum_required(VERSION 3.7)
project(ccsds_tm_packets)

# Set C++ version to c++17
set(CMAKE_CXX_STANDARD 17)

# Specify the directories for #includes
include_directories("${PROJECT_SOURCE_DIR}/inc" "${PROJECT_SOURCE_DIR}/lib/etl/include")

add_compile_options(-Wvla)

add_custom_target(check
        COMMAND ./cppcheck.sh
        COMMAND ./vera.sh
        COMMAND ./clang-tidy.sh
        COMMAND ./cppcheck-misra.sh
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ci")

# Specify the .cpp files common across all targets
add_library(common OBJECT
        src/Logger.cpp
        src/Platform/x86/Logger.cpp
        src/CCSDSServiceChannel.cpp
        src/CCSDSChannel.cpp
        src/FrameOperationProcedure.cpp
        src/FrameAcceptanceReporting.cpp
        src/logOperators.cpp
        src/MemoryPool.cpp
        src/CLCW.cpp
        )

# Specify the .cpp files for the executables
file(GLOB x86_main_SRC "src/Platform/x86/*.cpp")
add_executable(ccsds_tm_packets
        src/main.cpp
        $<TARGET_OBJECTS:common>
        #        ${x86_main_SRC}
        )

IF (EXISTS "${PROJECT_SOURCE_DIR}/lib/Catch2/CMakeLists.txt")
    # Gather all the .cpp files corresponding to tests
    file(GLOB test_main_SRC "test/*.cpp")
    file(GLOB test_SRC "test/**/*.cpp")

    add_subdirectory(lib/Catch2)
    add_executable(tests
            $<TARGET_OBJECTS:common>
            ${test_main_SRC}
            ${test_SRC})
    target_link_libraries(tests Catch2::Catch2WithMain)
ENDIF ()
set_target_properties(common PROPERTIES COMPILE_DEFINITIONS LOGLEVEL_TRACE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
