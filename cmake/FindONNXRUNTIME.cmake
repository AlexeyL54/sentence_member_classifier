# FindONNXRUNTIME.cmake
# При использовании vcpkg этот файл может даже не понадобиться,
# но оставим для совместимости с локальной сборкой

if(NOT TARGET onnxruntime)
    # Пробуем найти через config-mode (vcpkg предоставляет это)
    find_package(onnxruntime CONFIG QUIET)
    
    if(NOT onnxruntime_FOUND)
        # Fallback: ручной поиск через ENV переменную (для локальной разработки)
        if(NOT DEFINED onnxruntime_INCLUDE_DIRS)
            find_path(onnxruntime_INCLUDE_DIRS
                NAMES onnxruntime/onnxruntime_cxx_api.h
                PATHS $ENV{ONNXRUNTIME_ROOT} ${CMAKE_PREFIX_PATH}
                PATH_SUFFIXES include
            )
        endif()
        
        if(NOT DEFINED onnxruntime_LIB_DIRS)
            find_path(onnxruntime_LIB_DIRS
                NAMES onnxruntime.lib
                PATHS $ENV{ONNXRUNTIME_ROOT} ${CMAKE_PREFIX_PATH}
                PATH_SUFFIXES lib
            )
        endif()
        
        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(ONNXRUNTIME
            REQUIRED_VARS onnxruntime_INCLUDE_DIRS onnxruntime_LIB_DIRS
        )
        
        if(ONNXRUNTIME_FOUND AND NOT TARGET onnxruntime)
            add_library(onnxruntime INTERFACE IMPORTED)
            target_include_directories(onnxruntime INTERFACE ${onnxruntime_INCLUDE_DIRS})
            target_link_libraries(onnxruntime INTERFACE "${onnxruntime_LIB_DIRS}/onnxruntime.lib")
        endif()
    endif()
endif()
