cmake_minimum_required(VERSION 3.18.4)
project(test_package LANGUAGES CXX)

# Find the package being tested
find_package(ccsds-data-link-layer CONFIG REQUIRED COMPONENTS space_segment_lib ground_segment_lib)

# Test executable for space_segment
add_executable(test_space src/space_main.cpp)
target_link_libraries(test_space PRIVATE ccsds-data-link-layer::space_segment_lib)

# Test executable for ground_segment
add_executable(test_ground src/ground_main.cpp)
target_link_libraries(test_ground PRIVATE ccsds-data-link-layer::ground_segment_lib)