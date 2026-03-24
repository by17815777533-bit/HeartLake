cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0144 NEW)
project(HeartLake CXX)

include(CheckIPOSupported)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 默认使用 Release，避免测试/压测误落到无优化构建，和 2C2G 口径保持一致。
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)

check_ipo_supported(RESULT HEARTLAKE_IPO_SUPPORTED OUTPUT HEARTLAKE_IPO_ERROR)
if(HEARTLAKE_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
else()
    message(STATUS "IPO/LTO disabled: ${HEARTLAKE_IPO_ERROR}")
endif()

# 安全编译选项（栈保护 + FORTIFY_SOURCE + PIE）
add_compile_options(-fstack-protector-strong)
add_compile_definitions(
    $<$<CONFIG:Release>:_FORTIFY_SOURCE=2>
    $<$<CONFIG:RelWithDebInfo>:_FORTIFY_SOURCE=2>
    $<$<CONFIG:MinSizeRel>:_FORTIFY_SOURCE=2>
)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -Wl,-z,relro,-z,now")

# 可选：Sanitizer 构建（开发/测试用）
option(HEARTLAKE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(HEARTLAKE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(HEARTLAKE_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

if(HEARTLAKE_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "AddressSanitizer enabled")
endif()
if(HEARTLAKE_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()
if(HEARTLAKE_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
    message(STATUS "ThreadSanitizer enabled")
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
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
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
