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
option(HEARTLAKE_BUILD_TESTS "Build backend tests" ON)

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

if(HEARTLAKE_BUILD_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()

    set(TEST_SOURCES
        tests/test_edge_ai_engine.cpp
        tests/test_utils.cpp
        tests/test_security_crypto.cpp
        tests/test_differential_privacy.cpp
        tests/test_dual_memory_rag.cpp
        tests/test_emotion_resonance.cpp
        tests/test_advanced_embedding.cpp
        tests/test_circuit_breaker.cpp
        tests/test_psychological_risk_assessment.cpp
        tests/benchmark_edge_ai.cpp
        tests/stress_test.cpp
        tests/integration_test.cpp
        tests/controllers/stone_controller_test.cpp
        tests/services/stone_service_test.cpp
        tests/services/safe_harbor_service_test.cpp
        tests/test_validator_extended.cpp
        tests/test_id_generator_extended.cpp
        tests/test_edge_ai_engine_extended.cpp
        tests/test_security_crypto_extended.cpp
        tests/test_differential_privacy_extended.cpp
        tests/test_circuit_breaker_extended.cpp
        tests/test_dual_memory_rag_extended.cpp
        tests/test_emotion_resonance_extended.cpp
        tests/test_psychological_risk_extended.cpp
        tests/test_ai_torture.cpp
        tests/test_security_torture.cpp
        tests/test_concurrency_torture.cpp
        tests/filters/test_admin_auth_filter.cpp
        tests/filters/test_rate_limit_filter.cpp
        tests/filters/test_security_audit_filter.cpp
        tests/utils/test_rbac_manager.cpp
        tests/utils/test_paseto_extended.cpp
        tests/utils/test_recovery_key.cpp
        tests/utils/test_content_filter.cpp
        tests/utils/test_emotion_manager.cpp
        tests/utils/test_admin_config_store.cpp
        tests/utils/test_security_audit_score.cpp
        tests/utils/test_e2e_encryption_extended.cpp
        tests/utils/test_response_util.cpp
        tests/controllers/test_account_controller.cpp
        tests/controllers/test_user_controller.cpp
        tests/controllers/test_friend_controller.cpp
        tests/controllers/test_interaction_controller.cpp
        tests/controllers/test_admin_controller.cpp
        tests/controllers/test_recommendation_controller.cpp
        tests/controllers/test_consultation_controller.cpp
        tests/controllers/test_report_controller.cpp
        tests/controllers/test_guardian_controller.cpp
        tests/controllers/test_vip_controller.cpp
        tests/controllers/test_paper_boat_controller.cpp
        tests/controllers/test_temp_friend_controller.cpp
        tests/services/test_interaction_service.cpp
        tests/services/test_friend_service.cpp
        tests/services/test_user_service.cpp
        tests/websocket/test_websocket_hub.cpp
    )

    foreach(test_source ${TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)
        add_executable(${test_name} ${test_source})
        target_link_libraries(${test_name} PRIVATE ${PROJECT_NAME}_lib GTest::gtest GTest::gtest_main)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
