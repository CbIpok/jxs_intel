# Minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Project name and version
project(IntelJXS VERSION 1.0)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add executable and source files
add_subdirectory(decoder_sample)
# Optionally, add directories for additional include files
# include_directories(${PROJECT_SOURCE_DIR}/include)

# Link libraries if needed
# target_link_libraries(MyProject PRIVATE some_library)

# Optionally set compiler flags
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
