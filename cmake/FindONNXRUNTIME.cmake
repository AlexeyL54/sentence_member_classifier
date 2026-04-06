# FindONNXRUNTIME.cmake

# Сброс переменных перед поиском, чтобы избежать кэширования неверных путей при локальной разработке
# (Опционально, но полезно для чистоты)
# unset(onnxruntime_INCLUDE_DIRS CACHE)
# unset(onnxruntime_LIB_DIRS CACHE)

if(NOT DEFINED onnxruntime_INCLUDE_DIRS)
    message(STATUS "Searching for ONNX Runtime headers...")
    find_path(onnxruntime_INCLUDE_DIRS
        NAMES onnxruntime_cxx_api.h
        PATH_SUFFIXES include # Добавляем суффикс, если структура папок может варьироваться
        PATHS
            $ENV{ONNXRUNTIME_ROOT}/include
            ${CMAKE_PREFIX_PATH}/include
            "C:/Program Files/onnxruntime/include"
            "C:/onnxruntime/include"
            "${CMAKE_SOURCE_DIR}/../onnxruntime/include" # На случай локальной структуры
    )
endif()

if(NOT DEFINED onnxruntime_LIB_DIRS)
    message(STATUS "Searching for ONNX Runtime libs...")
    find_path(onnxruntime_LIB_DIRS
        NAMES onnxruntime.lib
        PATH_SUFFIXES lib
        PATHS
            $ENV{ONNXRUNTIME_ROOT}/lib
            ${CMAKE_PREFIX_PATH}/lib
            "C:/Program Files/onnxruntime/lib"
            "C:/onnxruntime/lib"
            "${CMAKE_SOURCE_DIR}/../onnxruntime/lib"
    )
endif()

message(STATUS "Found ONNX Runtime Include Dirs: ${onnxruntime_INCLUDE_DIRS}")
message(STATUS "Found ONNX Runtime Lib Dirs: ${onnxruntime_LIB_DIRS}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNXRUNTIME
    REQUIRED_VARS onnxruntime_INCLUDE_DIRS onnxruntime_LIB_DIRS
)

if(ONNXRUNTIME_FOUND)
    if(NOT TARGET onnxruntime)
        add_library(onnxruntime INTERFACE IMPORTED)
        
        # Важно: используем SYSTEM, чтобы подавить предупреждения из сторонних библиотек, 
        # и проверяем, что путь не пустой
        if(onnxruntime_INCLUDE_DIRS)
            target_include_directories(onnxruntime INTERFACE 
                ${onnxruntime_INCLUDE_DIRS}
            )
        else()
            message(FATAL_ERROR "ONNX Runtime include directory is empty!")
        endif()

        if(MSVC)
            set(ONNXRUNTIME_LIBRARY "${onnxruntime_LIB_DIRS}/onnxruntime.lib")
        else()
            # Для GCC/Clang на Windows или Linux имена могут отличаться
            set(ONNXRUNTIME_LIBRARY "${onnxruntime_LIB_DIRS}/onnxruntime.lib") 
        endif()

        # Проверка существования файла библиотеки
        if(EXISTS "${ONNXRUNTIME_LIBRARY}")
             target_link_libraries(onnxruntime INTERFACE "${ONNXRUNTIME_LIBRARY}")
        else()
             message(WARNING "ONNX Runtime library file not found at: ${ONNXRUNTIME_LIBRARY}")
             # Попробуем найти .dll.lib или просто .lib без пути, если нужно
             target_link_libraries(onnxruntime INTERFACE "${onnxruntime_LIB_DIRS}/onnxruntime.lib")
        endif()
    endif()
endif()
