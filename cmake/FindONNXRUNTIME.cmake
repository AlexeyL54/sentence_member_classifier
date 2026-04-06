# FindONNXRUNTIME.cmake

# Нормализация путей, переданных извне (убирает возможные лишние слеши и пробелы)
if(DEFINED onnxruntime_INCLUDE_DIRS)
    file(TO_CMAKE_PATH "${onnxruntime_INCLUDE_DIRS}" onnxruntime_INCLUDE_DIRS)
endif()

if(DEFINED onnxruntime_LIB_DIRS)
    file(TO_CMAKE_PATH "${onnxruntime_LIB_DIRS}" onnxruntime_LIB_DIRS)
endif()

if(NOT DEFINED onnxruntime_INCLUDE_DIRS OR onnxruntime_INCLUDE_DIRS STREQUAL "")
    message(STATUS "Searching for ONNX Runtime headers...")
    find_path(onnxruntime_INCLUDE_DIRS
        NAMES onnxruntime/onnxruntime_cxx_api.h # Ищем относительно корня include
        PATHS
            $ENV{ONNXRUNTIME_ROOT}
            ${CMAKE_PREFIX_PATH}
            "C:/Program Files/onnxruntime"
            "C:/onnxruntime"
        PATH_SUFFIXES include
    )
endif()

if(NOT DEFINED onnxruntime_LIB_DIRS OR onnxruntime_LIB_DIRS STREQUAL "")
    message(STATUS "Searching for ONNX Runtime libs...")
    find_path(onnxruntime_LIB_DIRS
        NAMES onnxruntime.lib
        PATHS
            $ENV{ONNXRUNTIME_ROOT}
            ${CMAKE_PREFIX_PATH}
            "C:/Program Files/onnxruntime"
            "C:/onnxruntime"
        PATH_SUFFIXES lib
    )
endif()

message(STATUS "Resolved ONNX Runtime Include Dirs: ${onnxruntime_INCLUDE_DIRS}")
message(STATUS "Resolved ONNX Runtime Lib Dirs: ${onnxruntime_LIB_DIRS}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRUNTIME
    REQUIRED_VARS onnxruntime_INCLUDE_DIRS onnxruntime_LIB_DIRS
)

if(ONNXRUNTIME_FOUND)
    if(NOT TARGET onnxruntime)
        add_library(onnxruntime INTERFACE IMPORTED)
        
        # Добавляем пути включения
        target_include_directories(onnxruntime INTERFACE
            ${onnxruntime_INCLUDE_DIRS}
        )

        # Формируем полный путь к библиотеке
        set(ONNXRUNTIME_LIBRARY "${onnxruntime_LIB_DIRS}/onnxruntime.lib")
        
        # Нормализуем путь к библиотеке для линковщика
        file(TO_NATIVE_PATH "${ONNXRUNTIME_LIBRARY}" ONNXRUNTIME_LIBRARY_NATIVE)

        target_link_libraries(onnxruntime INTERFACE
            "${ONNXRUNTIME_LIBRARY}"
        )

        message(STATUS "Linking against: ${ONNXRUNTIME_LIBRARY}")
        
        # Проверка наличия DLL для деплоя (не влияет на компиляцию, но полезно для логов)
        if(WIN32)
            set(ONNXRUNTIME_DLL_DIR "${onnxruntime_INCLUDE_DIRS}/../bin")
            file(TO_CMAKE_PATH "${ONNXRUNTIME_DLL_DIR}" ONNXRUNTIME_DLL_DIR)
            if(EXISTS "${ONNXRUNTIME_DLL_DIR}/onnxruntime.dll")
                message(STATUS "Found ONNX Runtime DLL: ${ONNXRUNTIME_DLL_DIR}/onnxruntime.dll")
            else()
                message(WARNING "ONNX Runtime DLL not found at: ${ONNXRUNTIME_DLL_DIR}/onnxruntime.dll")
            endif()
        endif()
    endif()
endif()
