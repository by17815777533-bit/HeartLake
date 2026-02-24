cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0144 NEW)
project(HeartLake CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Release optimization flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# ONNX Runtime (可选)
option(HEARTLAKE_USE_ONNX "Enable ONNX Runtime sentiment analysis" OFF)

# Find Drogon
find_package(Drogon CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

if(HEARTLAKE_USE_ONNX)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
    find_package(OnnxRuntime REQUIRED)
    message(STATUS "ONNX Runtime enabled: ${OnnxRuntime_LIBRARIES}")
endif()

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Collect all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*main\\.cpp$")

# 如果未启用 ONNX，排除 OnnxSentimentEngine.cpp
if(NOT HEARTLAKE_USE_ONNX)
    list(FILTER SOURCES EXCLUDE REGEX ".*OnnxSentimentEngine\\.cpp$")
endif()

# Create library for testing
add_library(${PROJECT_NAME}_lib STATIC ${SOURCES})
target_link_libraries(${PROJECT_NAME}_lib PUBLIC Drogon::Drogon OpenSSL::SSL OpenSSL::Crypto CURL::libcurl)

if(HEARTLAKE_USE_ONNX)
    target_link_libraries(${PROJECT_NAME}_lib PUBLIC OnnxRuntime::OnnxRuntime)
    target_compile_definitions(${PROJECT_NAME}_lib PUBLIC HEARTLAKE_USE_ONNX)
endif()

# Main executable
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE
    -Wl,--whole-archive ${PROJECT_NAME}_lib -Wl,--no-whole-archive
)

# Compiler warning flags
target_compile_options(${PROJECT_NAME}_lib PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
    -Wunused-parameter -Wfloat-conversion
)
target_compile_options(${PROJECT_NAME} PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wold-style-cast
    -Wunused-parameter -Wfloat-conversion
)
