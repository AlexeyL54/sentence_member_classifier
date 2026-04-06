cmake_minimum_required(VERSION 3.16)
project(TextAnalyzer VERSION 1.0.0)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Опции сборки
if(MSVC)
    add_compile_options(/W3 /MP)
    if(CMAKE_BUILD_TYPE MATCHES "Release")
        add_compile_options(/O2 /MT)
    else()
        add_compile_options(/Od /MTd)
    endif()
endif()

# Исходные файлы
set(SOURCES
    gui.cpp
    src/front/InputPage.cpp
    src/front/ResultPage.cpp
    src/front/SearchPage.cpp
    src/front/MainWindow.cpp
    src/front/lib/FlowLayout.cpp
    src/front/lib/TextMarkupWidget.cpp
    src/back/statistics.cpp
    src/back/bert_onnx_inference.cpp
    src/back/onnx_model.cpp
    src/back/cJSON.c
    src/back/simple_tokenizer.cpp
)

set(HEADERS
    src/front/InputPage.hpp
    src/front/ResultPage.hpp
    src/front/SearchPage.hpp
    src/front/MainWindow.hpp
    src/front/lib/FlowLayout.hpp
    src/front/lib/TextMarkupWidget.hpp
    src/back/statistics.hpp
    src/back/bert_onnx_inference.hpp
    src/back/onnx_model.hpp
    src/back/cJSON.h
    src/back/simple_tokenizer.hpp
)

# Поиск Qt
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# СНАЧАЛА создаем исполняемый файл
add_executable(TextAnalyzer WIN32 ${SOURCES} ${HEADERS})

# ПОТОМ добавляем include директории и линковку
if(WIN32)
    # Указываем путь к ONNX Runtime
    if(EXISTS "${CMAKE_SOURCE_DIR}/onnxruntime/include")
        set(ONNXRUNTIME_ROOT "${CMAKE_SOURCE_DIR}/onnxruntime")
        message(STATUS "Found ONNX Runtime at: ${ONNXRUNTIME_ROOT}")
    elseif(EXISTS "C:/onnxruntime")
        set(ONNXRUNTIME_ROOT "C:/onnxruntime")
        message(STATUS "Found ONNX Runtime at: ${ONNXRUNTIME_ROOT}")
    else()
        message(FATAL_ERROR "ONNX Runtime not found! Please place onnxruntime folder in project root or set ONNXRUNTIME_ROOT")
    endif()
    
    # Добавляем include директорию для компилятора
    target_include_directories(TextAnalyzer PRIVATE 
        "${ONNXRUNTIME_ROOT}/include"
    )
    
    # Добавляем библиотеку для линковки
    target_link_libraries(TextAnalyzer 
        Qt6::Core 
        Qt6::Widgets
        "${ONNXRUNTIME_ROOT}/lib/onnxruntime.lib"
    )
    
    # Копируем DLL после сборки
    if(EXISTS "${ONNXRUNTIME_ROOT}/bin/onnxruntime.dll")
        add_custom_command(TARGET TextAnalyzer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${ONNXRUNTIME_ROOT}/bin/onnxruntime.dll"
                $<TARGET_FILE_DIR:TextAnalyzer>
            COMMENT "Copying onnxruntime.dll to output directory"
        )
    endif()
else()
    # Linux/macOS
    find_package(onnxruntime REQUIRED)
    target_link_libraries(TextAnalyzer 
        Qt6::Core 
        Qt6::Widgets
        onnxruntime
    )
endif()

# Вывод информации о сборке
message(STATUS "Building TextAnalyzer version ${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Qt version: ${Qt6_VERSION}")
if(MSVC)
    message(STATUS "Compiler: MSVC ${MSVC_VERSION}")
    target_compile_definitions(TextAnalyzer PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
