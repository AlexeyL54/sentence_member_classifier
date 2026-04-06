# FindONNXRUNTIME.cmake

if(NOT DEFINED onnxruntime_INCLUDE_DIRS)
    find_path(onnxruntime_INCLUDE_DIRS
        NAMES onnxruntime/onnxruntime_cxx_api.h # Ищем вложенную папку
        PATHS
            $ENV{ONNXRUNTIME_ROOT}
            ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES include
    )
endif()

if(NOT DEFINED onnxruntime_LIB_DIRS)
    find_path(onnxruntime_LIB_DIRS
        NAMES onnxruntime.lib
        PATHS
            $ENV{ONNXRUNTIME_ROOT}
            ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES lib
    )
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRUNTIME
    REQUIRED_VARS onnxruntime_INCLUDE_DIRS onnxruntime_LIB_DIRS
)

if(ONNXRUNTIME_FOUND)
    if(NOT TARGET onnxruntime)
        add_library(onnxruntime INTERFACE IMPORTED)
        
        target_include_directories(onnxruntime INTERFACE
            ${onnxruntime_INCLUDE_DIRS}
        )

        set(ONNXRUNTIME_LIBRARY "${onnxruntime_LIB_DIRS}/onnxruntime.lib")
        target_link_libraries(onnxruntime INTERFACE "${ONNXRUNTIME_LIBRARY}")
    endif()
endif()
