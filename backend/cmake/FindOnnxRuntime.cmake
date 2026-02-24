# FindOnnxRuntime.cmake
# 查找 ONNX Runtime C++ 库
#
# 定义:
#   OnnxRuntime_FOUND        - 是否找到
#   OnnxRuntime_INCLUDE_DIRS - 头文件目录
#   OnnxRuntime_LIBRARIES    - 库文件
#   OnnxRuntime::OnnxRuntime - imported target

set(_ONNXRUNTIME_BUNDLED_GPU_ROOT "${CMAKE_SOURCE_DIR}/third_party/onnxruntime-linux-x64-gpu-1.22.0")
set(_ONNXRUNTIME_BUNDLED_CPU_ROOT "${CMAKE_SOURCE_DIR}/third_party/onnxruntime-linux-x64-1.22.0")

# Always refresh cache so switching CPU/GPU package works immediately.
unset(OnnxRuntime_INCLUDE_DIR CACHE)
unset(OnnxRuntime_LIBRARY CACHE)

# 搜索头文件（优先 bundled GPU，再到 CPU，再到系统）
find_path(OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATHS
        ${ONNXRUNTIME_ROOT}/include
        $ENV{ONNXRUNTIME_ROOT}/include
        ${_ONNXRUNTIME_BUNDLED_GPU_ROOT}/include
        ${_ONNXRUNTIME_BUNDLED_CPU_ROOT}/include
        /usr/local/include
        /usr/include
        /usr/local/include/onnxruntime
        /usr/include/onnxruntime
    PATH_SUFFIXES
        onnxruntime
        onnxruntime/core/session
)

# 搜索库文件（优先 bundled GPU，再到 CPU，再到系统）
find_library(OnnxRuntime_LIBRARY
    NAMES onnxruntime
    PATHS
        ${ONNXRUNTIME_ROOT}/lib
        $ENV{ONNXRUNTIME_ROOT}/lib
        ${_ONNXRUNTIME_BUNDLED_GPU_ROOT}/lib
        ${_ONNXRUNTIME_BUNDLED_CPU_ROOT}/lib
        /usr/local/lib
        /usr/lib
        /usr/local/lib64
        /usr/lib64
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime
    REQUIRED_VARS OnnxRuntime_LIBRARY OnnxRuntime_INCLUDE_DIR
)

if(OnnxRuntime_FOUND)
    set(OnnxRuntime_INCLUDE_DIRS ${OnnxRuntime_INCLUDE_DIR})
    set(OnnxRuntime_LIBRARIES ${OnnxRuntime_LIBRARY})

    get_filename_component(_ONNXRUNTIME_LIB_DIR "${OnnxRuntime_LIBRARY}" DIRECTORY)
    if(EXISTS "${_ONNXRUNTIME_LIB_DIR}/libonnxruntime_providers_cuda.so")
        set(OnnxRuntime_VARIANT "gpu")
    else()
        set(OnnxRuntime_VARIANT "cpu")
    endif()
    message(STATUS "OnnxRuntime variant: ${OnnxRuntime_VARIANT} (${_ONNXRUNTIME_LIB_DIR})")

    if(NOT TARGET OnnxRuntime::OnnxRuntime)
        add_library(OnnxRuntime::OnnxRuntime UNKNOWN IMPORTED)
        set_target_properties(OnnxRuntime::OnnxRuntime PROPERTIES
            IMPORTED_LOCATION "${OnnxRuntime_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OnnxRuntime_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(OnnxRuntime_INCLUDE_DIR OnnxRuntime_LIBRARY)
