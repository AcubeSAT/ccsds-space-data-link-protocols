cmake_minimum_required(VERSION 3.18.4)
project(ccsds-data-link-layer LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)

# ======================================== Compiler flags ========================================
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # This flag makes the compiler give warnings about variable length arrays. Example:
    # void foo(int n) {
    #   int arr[n];  // Dangerous for large n, due to stack overflow concerns in embedded systems
    # }
    add_compile_options(-Wvla)

    # Uncomment to throw error instead
    # add_compile_options(-Werror=vla)
endif()
add_compile_options(-fno-exceptions)

# ======================================== Build options ========================================
option(BUILD_SPACE_SEGMENT  "Build the space segment library" OFF)
option(BUILD_GROUND_SEGMENT "Build the ground segment library" OFF)
option(BUILD_UNIT_TESTS     "Build the unit tests" OFF)
option(BUILD_x86            "Include x86 files" ON)
option(FRAME_PRINTING_FUNCTIONS "Include functions that print frame fields" OFF)
set(PLATFORM_DEFINITIONS_PATH "" CACHE PATH "Path for platform-specific implementations")
set(CHANNEL_CONFIG            "" CACHE PATH "Path for channel configuration file")
set(KEYS_CONFIG               "" CACHE PATH "Path for security association keys file")

# ======================================== CI ========================================
# TODO
#add_custom_target(check
#        COMMAND ./cppcheck.sh
#        COMMAND ./vera.sh
#        COMMAND ./clang-tidy.sh
#        COMMAND ./cppcheck-misra.sh
#        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/ci")

# # ======================================== Packages ========================================
find_package(etl CONFIG REQUIRED)
find_package(logger CONFIG REQUIRED COMPONENTS log_common log_x86)
find_package(Catch2 CONFIG COMPONENTS Catch2::Catch2WithMain)

# set logger level (LOGLEVEL_TRACE shows all logs)
add_compile_definitions(LOGLEVEL_TRACE)

# tinycrypt uses make and does not accept a build output directory flag, so if make is called, it builds on source
# the generated library (.a) needs to be copied on build and then all the objects need to be removed from source

set(TINYCRYPT_SRC_DIR ${CMAKE_SOURCE_DIR}/lib/tinycrypt)

#   - File that will need to be copied
set(TINYCRYPT_LIB_NAME libtinycrypt.a)

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/lib/${TINYCRYPT_LIB_NAME}  # File that will be produced
        COMMAND ${CMAKE_MAKE_PROGRAM} CC=${CMAKE_C_COMPILER} OBJS='hmac.o hmac_prng.o sha256.o utils.o' ENABLE_TESTS=false # Run make
        COMMAND ${CMAKE_COMMAND} -E copy           # copy .a file from src dir to build dir
        ${TINYCRYPT_SRC_DIR}/lib/libtinycrypt.a ${CMAKE_BINARY_DIR}/lib/${TINYCRYPT_LIB_NAME}
        COMMAND make clean                         # run make clean on source to delete build artifacts
        WORKING_DIRECTORY ${TINYCRYPT_SRC_DIR}
        COMMENT "Building TinyCrypt library")

add_custom_target(tinycrypt_build DEPENDS ${CMAKE_BINARY_DIR}/lib/${TINYCRYPT_LIB_NAME}) # this is needed to trigger the custom command above

#   - Now expose it as an imported library
add_library(tinycrypt STATIC IMPORTED)
set_target_properties(tinycrypt PROPERTIES
        IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib/${TINYCRYPT_LIB_NAME}
        INTERFACE_INCLUDE_DIRECTORIES ${TINYCRYPT_SRC_DIR}/lib/include)

#   - Make tinycrypt_build target run before anyone links tinycrypt
add_dependencies(tinycrypt tinycrypt_build)

# ==================================== Collect headers, sources, configs ====================================
# Specify include directories common for every target
set(COMMON_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/Channels"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/COP1"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/DataHandlingFunctions"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/DataStructures"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/NotificationAndLoggingUtilities"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/Services"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/GeneratedObjects"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc/Utilities")

# Specify sources common for every target
file(GLOB COMMON_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/COP1/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/DataHandlingFunctions/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/NotificationAndLoggingUtilities/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Services/*.cpp")

# Unit test sources
file(GLOB UNIT_TESTS
        "${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/DataStructures/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/DataHandlingFunctions/*.cpp")

# Platform directories and sources
if(BUILD_x86)
    message("Using x86 platform definitions")
    set(PLATFORM_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/inc/Platform/x86")
    file(GLOB_RECURSE PLATFORM_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/x86/*.cpp")
else()
    message("Using user provided platform definitions path: ${PLATFORM_DEFINITIONS_PATH}")
    set(PLATFORM_INCLUDE_DIRS "${PLATFORM_DEFINITIONS_PATH}" "${PLATFORM_DEFINITIONS_PATH}/inc")
    file(GLOB_RECURSE PLATFORM_SOURCES "${PLATFORM_DEFINITIONS_PATH}/*.cpp")
endif()

# Channel configuration and key files
if(BUILD_UNIT_TESTS)
    # Use unit test channel config
    message("Using unit testing channel and key configurations")
    set(CHANNEL_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/test/CCSDSDataLink.def)
    set(KEYS_CONFIG    ${CMAKE_CURRENT_SOURCE_DIR}/test/CCSDSKeys.def)
else()
    if("${CHANNEL_CONFIG}" STREQUAL "")
        # User did not provide a channel configuration, fallback to a default one
        message("Using default channel configuration")
        set(CHANNEL_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/inc/Platform/CCSDSDataLink.def)
    else()
        # Check if user provided config is valid
        if(CHANNEL_CONFIG MATCHES "CCSDSDataLink\\.def$")
            message("Using provided channel configuration: ${CHANNEL_CONFIG}")
        else()
            message(FATAL_ERROR "Provided channel configuration is invalid: ${CHANNEL_CONFIG}")
        endif()
    endif()

    if("${KEYS_CONFIG}" STREQUAL "")
        # User did not provide keys configuration, fallback to a default one
        message("Using default keys")
        set(KEYS_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/inc/Platform/CCSDSKeys.def)
    else()
        # Check if user provided keys config is valid
        if(KEYS_CONFIG MATCHES "CCSDSKeys\\.def$")
            message("Using provided keys configuration: ${KEYS_CONFIG}")
        else()
            message(FATAL_ERROR "Provided keys configuration is invalid: ${KEYS_CONFIG}")
        endif()
    endif()
endif()

# ======================================== Build targets ========================================

# Function that performs common actions for every target
function(common_actions target)
    # depending on the CHANNEL_CONFIG provided, create the ChannelGeneration header
    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/inc/GeneratedObjects/ChannelGeneration.hpp.in
            ${CMAKE_CURRENT_SOURCE_DIR}/inc/GeneratedObjects/ChannelGeneration.hpp
            @ONLY
    )

    configure_file(
            ${CMAKE_CURRENT_SOURCE_DIR}/inc/GeneratedObjects/SecurityAssociationKeys.hpp.in
            ${CMAKE_CURRENT_SOURCE_DIR}/inc/GeneratedObjects/SecurityAssociationKeys.hpp
            @ONLY
    )

    target_link_libraries(${target} PUBLIC etl::etl tinycrypt log_common)
    target_include_directories(${target} PUBLIC ${COMMON_INCLUDE_DIRS} ${PLATFORM_INCLUDE_DIRS})
    target_sources(${target} PRIVATE ${COMMON_SOURCES} ${PLATFORM_SOURCES})
    if(BUILD_x86)
        target_link_libraries(${target} PRIVATE log_x86)
    endif()
endfunction()

# include libs, headers and sources for each target
if(BUILD_SPACE_SEGMENT)
    add_library(space_segment_lib STATIC)
    target_compile_definitions(space_segment_lib PRIVATE INCLUDE_SPACE_SEGMENT_CODE)
    if(FRAME_PRINTING_FUNCTIONS)
        target_compile_definitions(space_segment_lib PUBLIC INCLUDE_FRAME_PRINTING_FUNCTIONS)
    endif()
    common_actions(space_segment_lib)
    install(TARGETS space_segment_lib)
endif()

if(BUILD_GROUND_SEGMENT)
    add_library(ground_segment_lib STATIC)
    target_compile_definitions(ground_segment_lib PUBLIC INCLUDE_GROUND_SEGMENT_CODE)
    if(FRAME_PRINTING_FUNCTIONS)
        target_compile_definitions(ground_segment_lib PUBLIC INCLUDE_FRAME_PRINTING_FUNCTIONS)
    endif()
    common_actions(ground_segment_lib)
    install(TARGETS ground_segment_lib)
endif()

if(BUILD_UNIT_TESTS)
    if (NOT TARGET Catch2::Catch2WithMain)
        message(FATAL_ERROR "BUILD_UNIT_TESTS=ON but Catch2::Catch2WithMain not found")
    endif()

    add_executable(catch2_unit_testing)
    target_compile_definitions(catch2_unit_testing PUBLIC INCLUDE_SPACE_SEGMENT_CODE
                                                          INCLUDE_GROUND_SEGMENT_CODE
                                                          INCLUDE_FRAME_PRINTING_FUNCTIONS)
    target_link_libraries(catch2_unit_testing PRIVATE Catch2::Catch2WithMain)
    target_sources(catch2_unit_testing PRIVATE ${UNIT_TESTS})
    common_actions(catch2_unit_testing)
endif()