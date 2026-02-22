# FindOnnxRuntime.cmake
# 查找 ONNX Runtime C++ 库
#
# 定义:
#   OnnxRuntime_FOUND        - 是否找到
#   OnnxRuntime_INCLUDE_DIRS - 头文件目录
#   OnnxRuntime_LIBRARIES    - 库文件
#   OnnxRuntime::OnnxRuntime - imported target

# 搜索头文件
find_path(OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_cxx_api.h
    PATHS
        /usr/local/include
        /usr/include
        /usr/local/include/onnxruntime
        /usr/include/onnxruntime
        ${ONNXRUNTIME_ROOT}/include
        $ENV{ONNXRUNTIME_ROOT}/include
    PATH_SUFFIXES
        onnxruntime
        onnxruntime/core/session
)

# 搜索库文件
find_library(OnnxRuntime_LIBRARY
    NAMES onnxruntime
    PATHS
        /usr/local/lib
        /usr/lib
        /usr/local/lib64
        /usr/lib64
        ${ONNXRUNTIME_ROOT}/lib
        $ENV{ONNXRUNTIME_ROOT}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime
    REQUIRED_VARS OnnxRuntime_LIBRARY OnnxRuntime_INCLUDE_DIR
)

if(OnnxRuntime_FOUND)
    set(OnnxRuntime_INCLUDE_DIRS ${OnnxRuntime_INCLUDE_DIR})
    set(OnnxRuntime_LIBRARIES ${OnnxRuntime_LIBRARY})

    if(NOT TARGET OnnxRuntime::OnnxRuntime)
        add_library(OnnxRuntime::OnnxRuntime UNKNOWN IMPORTED)
        set_target_properties(OnnxRuntime::OnnxRuntime PROPERTIES
            IMPORTED_LOCATION "${OnnxRuntime_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OnnxRuntime_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(OnnxRuntime_INCLUDE_DIR OnnxRuntime_LIBRARY)
