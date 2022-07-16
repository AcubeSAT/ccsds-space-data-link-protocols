cmake_minimum_required(VERSION 3.7)

SET(CMAKE_C_COMPILER_WORKS 1)
SET(CMAKE_C_COMPILER arm-none-eabi-gcc)
SET(CMAKE_CXX_COMPILER_WORKS 1)
SET(CMAKE_CXX_COMPILER arm-none-eabi-g++)

project(ccsds_tm_packets)

set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(SIZE arm-none-eabi-size)

# Set C++ version to c++17
set(CMAKE_CXX_STANDARD 17)

# Specify the directories for #includes
include_directories("${PROJECT_SOURCE_DIR}/inc" "${PROJECT_SOURCE_DIR}/lib/etl/include")
include_directories(inc)
include_directories(inc/FreeRTOSTasks)
include_directories(src)
include_directories(src/FreeRTOSTasks)
include_directories(lib/FreeRTOS)
include_directories(lib/FreeRTOS/Source)
include_directories(lib/FreeRTOS/Source/include)
include_directories(lib/FreeRTOS/Source/portable)
include_directories(lib/FreeRTOS/Source/portable/GCC)
include_directories(lib/FreeRTOS/Source/portable/GCC/ARM_CM7)
include_directories(lib/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1)
include_directories(lib/FreeRTOS/Source/portable/MemMang)


add_compile_options(-mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv4-sp-d16)
add_link_options(-mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv4-sp-d16)


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
        src/Packet.cpp
        src/logOperators.cpp src/MemoryPool.cpp src/CLCW.cpp)

# Specify the .cpp files for the executables
file(GLOB x86_main_SRC "src/Platform/x86/*.cpp")
file(GLOB_RECURSE FREERTOS_SOURCES "lib/FreeRTOS/*.c")
file(GLOB_RECURSE FREERTOS_TASKS "src/FreeRTOSTasks/*.c" "src/FreeRTOSTasks/*.cpp")

add_executable(ccsds_tm_packets
        ${FREERTOS_SOURCES}
        ${FREERTOS_TASKS}
        src/main.cpp
        src/main.c
        $<TARGET_OBJECTS:common>
        #        ${x86_main_SRC}
        inc/Alert.hpp inc/TransferFrame.hpp inc/TransferFrameTM.hpp inc/Logger.hpp inc/CCSDSLogger.h inc/CCSDSLoggerImpl.h inc/logOperators.h
        inc/MemoryPool.hpp inc/CLCW.hpp)

IF (EXISTS "${PROJECT_SOURCE_DIR}/lib/Catch2/CMakeLists.txt")
    # Gather all the .cpp files corresponding to tests
    file(GLOB test_main_SRC "test/*.cpp")
    file(GLOB test_SRC "test/**/*.cpp")

    add_subdirectory(lib/Catch2)
    add_executable(tests
            $<TARGET_OBJECTS:common>
            ${test_main_SRC}
            ${test_SRC} )
    target_link_libraries(tests Catch2::Catch2)
ENDIF ()
set_target_properties(common PROPERTIES COMPILE_DEFINITIONS LOGLEVEL_TRACE)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ")

target_link_options(ccsds_tm_packets PRIVATE "-specs=nosys.specs")