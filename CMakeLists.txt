cmake_minimum_required(VERSION 3.30.5)
project(ccsds_tm_packets)

# Set C++ version to c++17
set(CMAKE_CXX_STANDARD 17)

# Specify the directories for #includes
include_directories("${PROJECT_SOURCE_DIR}/inc"
                    "${PROJECT_SOURCE_DIR}/lib/etl/include"
                    "${PROJECT_SOURCE_DIR}/inc/Platform/x86"
                    "${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/include")

add_compile_options(-Wvla)

add_custom_target(check
        COMMAND ./cppcheck.sh
        COMMAND ./vera.sh
        COMMAND ./clang-tidy.sh
        COMMAND ./cppcheck-misra.sh
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ci")

add_custom_target(
        TinyCryptLib ALL
        COMMAND make ENABLE_TESTS='false' OBJS='hmac.o hmac_prng.o sha256.o utils.o'
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/tinycrypt
)

# Specify the .cpp files common across all targets
add_library(common OBJECT
        src/Logger.cpp
        src/Platform/x86/Logger.cpp
        src/CCSDSServiceChannel.cpp
        src/CCSDSChannel.cpp
        src/FrameOperationProcedure.cpp
        src/FopStateTable.cpp
        src/FrameAcceptanceReporting.cpp
        src/logOperators.cpp
        src/MemoryPool.cpp
        src/CCSDSSecurityAssociation.cpp
        )

# Specify the .cpp files for the executables
add_executable(ccsds_tm_packets_default
        $<TARGET_OBJECTS:common>
        src/main.cpp
        )
add_dependencies(ccsds_tm_packets_default TinyCryptLib) # ensure "TinyCryptLib" target is built first
target_link_libraries(ccsds_tm_packets_default PRIVATE ${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/libtinycrypt.a)

add_executable(ccsds_tm_packets_yamcs_connection
        $<TARGET_OBJECTS:common>
        src/Platform/x86/FrameMaker.cpp
        src/Platform/x86/FrameSender.cpp
        src/Platform/x86/main.cpp
        )
target_compile_definitions(ccsds_tm_packets_yamcs_connection PRIVATE ENABLE_BUFFER_ACCESS)
add_dependencies(ccsds_tm_packets_yamcs_connection TinyCryptLib) # ensure "TinyCryptLib" target is built first
target_link_libraries(ccsds_tm_packets_yamcs_connection PRIVATE ${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/libtinycrypt.a)

IF (EXISTS "${PROJECT_SOURCE_DIR}/lib/Catch2/CMakeLists.txt")
    # Gather all the .cpp files corresponding to tests
    file(GLOB test_main_SRC "test/*.cpp")
    file(GLOB test_SRC "test/**/*.cpp")

    add_subdirectory(lib/Catch2)
    add_executable(tests
            $<TARGET_OBJECTS:common>
           # ${test_main_SRC}
           # ${test_SRC}
    )
    target_compile_definitions(tests PRIVATE ENABLE_BUFFER_ACCESS)
    add_dependencies(tests TinyCryptLib) # ensure "TinyCryptLib" target is built first
    target_link_libraries(tests Catch2::Catch2WithMain  ${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/libtinycrypt.a)
ENDIF ()
set_target_properties(common PROPERTIES COMPILE_DEFINITIONS LOGLEVEL_TRACE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")
