cmake_minimum_required(VERSION 3.15)
project(glsl-path-tracer)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# glfw
message("Configuring glfw...")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)

# glad
message("Configuring glad...")
add_subdirectory(third_party/glad)

add_definitions(-DROOT_FOLDER="${CMAKE_SOURCE_DIR}/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(third_party/glm)
include_directories(third_party/)
include_directories(source/)
add_subdirectory(source)
