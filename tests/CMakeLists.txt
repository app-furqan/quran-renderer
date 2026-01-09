cmake_minimum_required(VERSION 3.10)
project(quran_renderer_tests)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the test executable
add_executable(test_landscape_spacing test_landscape_spacing.cpp)

# Include directories
target_include_directories(test_landscape_spacing PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Link against the main library (if building as part of main project)
# For standalone test, we'll just compile with the headers
