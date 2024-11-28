cmake_minimum_required(VERSION 3.10)
project(monocular_depth)

# Find OpenCV package
find_package(OpenCV REQUIRED)

# Include OpenCV headers
include_directories(/usr/local/include/opencv4/opencv2)
include_directories(${OpenCV_INCLUDE_DIRS})

# Add executable
add_executable(monocular_depth ./main.cpp)

# Link libraries
target_link_libraries(monocular_depth ${OpenCV_LIBS})