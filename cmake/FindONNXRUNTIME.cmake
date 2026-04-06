# FindONNXRUNTIME.cmake
# Модуль для поиска ONNX Runtime при сборке на Windows

# Переменные, которые можно задать извне:
#   onnxruntime_INCLUDE_DIRS - путь к include (например, .../onnxruntime/include)
#   onnxruntime_LIB_DIRS     - путь к lib (например, .../onnxruntime/lib)

if(NOT DEFINED onnxruntime_INCLUDE_DIRS)
    # Пытаемся найти стандартные пути
    find_path(onnxruntime_INCLUDE_DIRS
        NAMES onnxruntime_cxx_api.h
        PATHS
            $ENV{ONNXRUNTIME_ROOT}/include
            ${CMAKE_PREFIX_PATH}/include
            C:/Program Files/onnxruntime/include
            "C:/onnxruntime/include"
    )
endif()

if(NOT DEFINED onnxruntime_LIB_DIRS)
    find_path(onnxruntime_LIB_DIRS
        NAMES onnxruntime.lib
        PATHS
            $ENV{ONNXRUNTIME_ROOT}/lib
            ${CMAKE_PREFIX_PATH}/lib
            C:/Program Files/onnxruntime/lib
            "C:/onnxruntime/lib"
    )
endif()

# Проверяем, что найдено
include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(ONNXRUNTIME
    REQUIRED_VARS onnxruntime_INCLUDE_DIRS onnxruntime_LIB_DIRS
)

if(ONNXRUNTIME_FOUND)
    # Создаем импортированную библиотеку
    if(NOT TARGET onnxruntime)
        add_library(onnxruntime INTERFACE IMPORTED)

        target_include_directories(onnxruntime INTERFACE
            ${onnxruntime_INCLUDE_DIRS}
        )

        # Для MSVC используем .lib файл
        if(MSVC)
            set(ONNXRUNTIME_LIBRARY "${onnxruntime_LIB_DIRS}/onnxruntime.lib")
        else()
            set(ONNXRUNTIME_LIBRARY "${onnxruntime_LIB_DIRS}/onnxruntime.lib")
        endif()

        target_link_libraries(onnxruntime INTERFACE
            ${ONNXRUNTIME_LIBRARY}
        )

        # Копируем DLL в выходную директорию при сборке
        if(WIN32)
            set(ONNXRUNTIME_DLL_DIR "${onnxruntime_INCLUDE_DIRS}/../bin")
            if(EXISTS "${ONNXRUNTIME_DLL_DIR}/onnxruntime.dll")
                message(STATUS "Found ONNX Runtime DLL: ${ONNXRUNTIME_DLL_DIR}/onnxruntime.dll")
            endif()
        endif()
    endif()

    message(STATUS "Found ONNX Runtime:")
    message(STATUS "  Include dirs: ${onnxruntime_INCLUDE_DIRS}")
    message(STATUS "  Library dirs: ${onnxruntime_LIB_DIRS}")
endif()
